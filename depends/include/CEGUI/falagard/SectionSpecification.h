/***********************************************************************
    created:    Mon Jun 13 2005
    author:     Paul D Turner <paul@cegui.org.uk>
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
#ifndef _CEGUIFalSectionSpecification_h_
#define _CEGUIFalSectionSpecification_h_

#include "../Window.h"
#include "../ColourRect.h"


// Start of CEGUI namespace section
namespace CEGUI
{
    // forward refs
    class WidgetLookFeel;

    /*!
    \brief
        Class that represents a simple 'link' to an ImagerySection.

        This class enables sections to be easily re-used, by different states and/or layers, by allowing
        sections to be specified by name rather than having mutiple copies of the same thing all over the place.
    */
    class CEGUIEXPORT SectionSpecification :
        public AllocatedObject<SectionSpecification>
    {
    public:
        SectionSpecification();

        /*!
        \brief
            Constructor

        \param owner
            String holding the name of the WidgetLookFeel object that contains the target section.

        \param sectionName
            String holding the name of the target section.

        \param controlPropertySource
            String holding the name of a property that will control whether
            rendering for this secion will actually occur or not.

        \param controlPropertyValue
            String holding the value to be tested for from the property named in
            controlPropertySource.  If this is empty, then controlPropertySource
            will be accessed as a boolean property, otherwise rendering will
            only occur when the value returned via controlPropertySource matches
            the value specified here.

        \param controlPropertyWidget
            String holding either a child widget name or the special value of
            '__parent__' indicating the window upon which the property named
            in controlPropertySource should be accessed.  If this is empty then
            the window itself is used as the source, rather than a child or the
            parent.
        */
        SectionSpecification(const String& owner, const String& sectionName,
                             const String& controlPropertySource,
                             const String& controlPropertyValue,
                             const String& controlPropertyWidget);

        /*!
        \brief
            Constructor

        \param owner
            String holding the name of the WidgetLookFeel object that contains the target section.

        \param sectionName
            String holding the name of the target section.

        \param controlPropertySource
            String holding the name of a property that will control whether
            rendering for this secion will actually occur or not.

        \param controlPropertyValue
            String holding the value to be tested for from the property named in
            controlPropertySource.  If this is empty, then controlPropertySource
            will be accessed as a boolean property, otherwise rendering will
            only occur when the value returned via controlPropertySource matches
            the value specified here.

        \param controlPropertyWidget
            String holding either a child widget name or the special value of
            '__parent__' indicating the window upon which the property named
            in controlPropertySource should be accessed.  If this is empty then
            the window itself is used as the source, rather than a child or the
            parent.

        \param cols
            Override colours to be used (modulates sections master colours).
        */
        SectionSpecification(const String& owner, const String& sectionName,
                             const String& controlPropertySource,
                             const String& controlPropertyValue,
                             const String& controlPropertyWidget,
                             const ColourRect& cols);

        /*!
        \brief
            Render the section specified by this SectionSpecification.

        \param srcWindow
            Window object to be used when calculating pixel values from BaseDim values.

        \return
            Nothing.
        */
        void render(Window& srcWindow, const ColourRect* modcols = 0, const Rectf* clipper = 0, bool clipToDisplay = false) const;

        /*!
        \brief
            Render the section specified by this SectionSpecification.

        \param srcWindow
            Window object to be used when calculating pixel values from BaseDim values.

        \param baseRect
            Rect object to be used when calculating pixel values from BaseDim values.

        \return
            Nothing.
        */
        void render(Window& srcWindow, const Rectf& baseRect, const ColourRect* modcols = 0, const Rectf* clipper = 0, bool clipToDisplay = false) const;

        /*!
        \brief
            Return the name of the WidgetLookFeel object containing the target section.

        \return
            String object holding the name of the WidgetLookFeel that contains the target ImagerySection.
        */
        const String& getOwnerWidgetLookFeel() const;

        /*!
        \brief
            Return the name of the WidgetLookFeel object containing the target section.

        \param name
            String object holding the name of the WidgetLookFeel that contains the target ImagerySection.

        \return
            Nothing.
        */
        void setOwnerWidgetLookFeel(const String& owner);

        /*!
        \brief
            Return the name of the target ImagerySection.

        \return
            String object holding the name of the target ImagerySection.
        */
        const String& getSectionName() const;
        /*!
        \brief
            Return the name of the target ImagerySection.

        \param name
            String object holding the name of the target ImagerySection.

        \return
            Nothing.
        */
        void setSectionName(const String& name);

        /*!
        \brief
            Return the current override colours.

        \return
            ColourRect holding the colours that will be modulated with the sections master colours if
            colour override is enabled on this SectionSpecification.
        */
        const ColourRect& getOverrideColours() const;

        /*!
        \brief
            Set the override colours to be used by this SectionSpecification.

        \param cols
            ColourRect describing the override colours to set for this SectionSpecification.

        \return
            Nothing.
        */
        void setOverrideColours(const ColourRect& cols);

        /*!
        \brief
            return whether the use of override colours is enabled on this SectionSpecification.

        \return
            - true if override colours will be used for this SectionSpecification.
            - false if override colours will not be used for this SectionSpecification.
        */
        bool isUsingOverrideColours() const;

        /*!
        \brief
            Enable or disable the use of override colours for this section.

        \param setting
            - true if override colours should be used for this SectionSpecification.
            - false if override colours should not be used for this SectionSpecification.

        \return
            Nothing.
        */
        void setUsingOverrideColours(bool setting = true);
        /*!
        \brief
            Get the name of the property where override colour values can be obtained.

        \return
            String containing the name of the property.
        */
        const String& getOverrideColoursPropertySource() const;

        /*!
        \brief
            Set the name of the property where override colour values can be obtained.

        \param property
            String containing the name of the property.

        \return
            Nothing.
        */
        void setOverrideColoursPropertySource(const String& property);

        /*!
        \brief
            Get the name of the property that controls whether to actually
            render this section.

        \return
            String containing the name of the property.
        */
        const String& getRenderControlPropertySource() const;

        /*!
        \brief
            Set the name of the property that controls whether to actually
            render this section.

        \param property
            String containing the name of the property.

        \return
            Nothing.
        */
        void setRenderControlPropertySource(const String& property);

        /*!
        \brief
            Get the test value used when determining whether to render this
            section.

            The value set here will be compared to the current value of the
            property named as the render control property, if they match the
            secion will be drawn, otherwise the section will not be drawn.  If
            this value is set to the empty string, the control property will
            instead be treated as a boolean property.
        */
        const String& getRenderControlValue() const;

        /*!
        \brief
            Set the test value used when determining whether to render this
            section.
            
            The value set here will be compared to the current value of the
            property named as the render control property, if they match the
            secion will be drawn, otherwise the section will not be drawn.  If
            this value is set to the empty string, the control property will
            instead be treated as a boolean property.
        */
        void setRenderControlValue(const String& value);

        /*!
        \brief
            Get the widget what will be used as the source of the property
            named as the control property.

            The value of this setting will be interpreted as follows:
            - empty string: The target widget being drawn will be the source of
              the property value.
            - '__parent__': The parent of the widget being drawn will be the
              source of the property value.
            - any other value: The value will be taken as a name and
              a child window with the specified name will be the source of the
              property value.
        */
        const String& getRenderControlWidget() const;

        /*!
        \brief
            Set the widget what will be used as the source of the property
            named as the control property.

            The value of this setting will be interpreted as follows:
            - empty string: The target widget being drawn will be the source of
              the property value.
            - '__parent__': The parent of the widget being drawn will be the
              source of the property value.
            - any other value: The value will be taken as a name and
              a child window with the specified name will be the source of the
              property value.
        */
        void setRenderControlWidget(const String& widget);

        /*!
        \brief
            Writes an xml representation of this SectionSpecification to \a out_stream.

        \param xml_stream
            Stream where xml data should be output.


        \return
            Nothing.
        */
        void writeXMLToStream(XMLSerializer& xml_stream) const;

    protected:
        /*!
        \brief
            Helper method to initialise a ColourRect with appropriate values according to the way the
            section sepcification is set up.

            This will try and get values from multiple places:
                - a property attached to \a wnd
                - the integral d_coloursOverride values.
                - or default to colour(1,1,1,1);
        */
        void initColourRectForOverride(const Window& wnd, ColourRect& cr) const;

        /** return whether the section should be drawn, based upon the
         * render control property and associated items.
         */
        bool shouldBeDrawn(const Window& wnd) const;

    private:
        String          d_owner;                //!< Name of the WidgetLookFeel containing the required section.
        String          d_sectionName;          //!< Name of the required section within the specified WidgetLookFeel.
        ColourRect      d_coloursOverride;      //!< Colours to use when override is enabled.
        bool            d_usingColourOverride;  //!< true if colour override is enabled.
        String          d_colourPropertyName;   //!< name of property to fetch colours from.
        //! Name of a property to control whether to draw this section.
        String d_renderControlProperty;
        //! Comparison value to test against d_renderControlProperty.
        String d_renderControlValue;
        //! Widget upon which d_renderControlProperty is to be accessed.
        String d_renderControlWidget;
    };


} // End of  CEGUI namespace section


#endif  // end of guard _CEGUIFalSectionSpecification_h_
