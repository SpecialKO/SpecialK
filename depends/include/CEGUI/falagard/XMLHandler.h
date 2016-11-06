/***********************************************************************
    created:    Fri Jun 17 2005
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
#ifndef _CEGUIFalagard_xmlHandler_h_
#define _CEGUIFalagard_xmlHandler_h_

#include "../ChainedXMLHandler.h"
#include "./Dimensions.h"
#include "../Window.h"
#include <vector>

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
    // forward refs
    class WidgetLookManager;
    class WidgetLookFeel;
    class WidgetComponent;
    class ImagerySection;
    class StateImagery;
    class LayerSpecification;
    class SectionSpecification;
    class ImageryComponent;
    class ComponentArea;
    class Dimension;
    class TextComponent;
    class NamedArea;
    class FrameComponent;
    class PropertyDefinitionBase;
    class EventLinkDefinition;

    /*!
    \brief
        Handler class used to parse look & feel XML files used by the Falagard system.
    */
    class CEGUIEXPORT Falagard_xmlHandler : public ChainedXMLHandler
    {
    public:
        /*!
        \brief
            Constructor for Falagard_xmlHandler objects
        */
        Falagard_xmlHandler(WidgetLookManager* mgr);

        /*!
        \brief
            Destructor for Falagard_xmlHandler objects
        */
        ~Falagard_xmlHandler();
        
        //! Stores the native version, the only version we are supposed to load
        static const String NativeVersion;

        //! Default value for the "type" attribute of PropertyDefinition elements
        static const String PropertyDefinitionHelpDefaultValue;

        //! Default value for the "type" attribute of PropertyLinkDefinition elements
        static const String PropertyLinkDefinitionHelpDefaultValue;

        //! Default or unspecified value for the "dataType" attribute
        static const String GenericDataType;


        //! String value representing a parent link identifier
        static const String ParentIdentifier;

        /*************************************************************************
            XML element and attribute name constants
        *************************************************************************/
        // element names
        static const String FalagardElement;            //!< Tag name for root Falagard elements.
        static const String WidgetLookElement;          //!< Tag name for WidgetLook elements.
        static const String ChildElement;               //!< Tag name for Child elements.
        static const String ImagerySectionElement;      //!< Tag name for ImagerySection elements.
        static const String StateImageryElement;        //!< Tag name for StateImagery elements.
        static const String LayerElement;               //!< Tag name for Layer elements.
        static const String SectionElement;             //!< Tag name for Section elements.
        static const String ImageryComponentElement;    //!< Tag name for ImageryComponent elements.
        static const String TextComponentElement;       //!< Tag name for TextComponent elements.
        static const String FrameComponentElement;      //!< Tag name for FrameComponent elements.
        static const String AreaElement;                //!< Tag name for Area elements.
        static const String ImageElement;               //!< Tag name for Image elements.
        static const String ColoursElement;             //!< Tag name for Colours elements.
        static const String VertFormatElement;          //!< Tag name for VertFormat elements.
        static const String HorzFormatElement;          //!< Tag name for HorzFormat elements.
        static const String VertAlignmentElement;       //!< Tag name for VertAlignment elements.
        static const String HorzAlignmentElement;       //!< Tag name for HorzAlignment elements.
        static const String PropertyElement;            //!< Tag name for Property elements.
		static const String DimElement;                 //!< Tag name for dimension container elements.
		static const String UnifiedDimElement;          //!< Tag name for unified dimension elements.
        static const String AbsoluteDimElement;         //!< Tag name for absolute dimension elements.
        static const String ImageDimElement;            //!< Tag name for image dimension elements.
        static const String ImagePropertyDimElement;    //!< Tag name for image property dimension elements.
        static const String WidgetDimElement;           //!< Tag name for widget dimension elements.
        static const String FontDimElement;             //!< Tag name for font dimension elements.
        static const String PropertyDimElement;         //!< Tag name for property dimension elements.
        static const String TextElement;                //!< Tag name for text component text elements
        static const String ColourPropertyElement;      //!< Tag name for property colour elements (fetches cols from a colour property)
        static const String ColourRectPropertyElement;  //!< Tag name for property colour elements (fetches cols from a ColourRect property)
        static const String NamedAreaElement;           //!< Tag name for named area elements.
        static const String PropertyDefinitionElement;  //!< Tag name for property definition elements.
        static const String PropertyLinkDefinitionElement;  //!< Tag name for property link elements.
        static const String PropertyLinkTargetElement;  //!< Tag name for property link target elements.
        static const String OperatorDimElement;         //!< Tag name for dimension operator elements.
        static const String VertFormatPropertyElement;  //!< Tag name for element that specifies a vertical formatting property.
        static const String HorzFormatPropertyElement;  //!< Tag name for element that specifies a horizontal formatting property..
        static const String AreaPropertyElement;        //!< Tag name for element that specifies a URect property..
        static const String ImagePropertyElement;       //!< Tag name for element that specifies an Image property..
        static const String TextPropertyElement;        //!< Tag name for element that specifies an Text property.
        static const String FontPropertyElement;        //!< Tag name for element that specifies an Font property.
        static const String ColourElement;              //!< Tag name for Colour elements.
        static const String EventLinkDefinitionElement; //!< Tag name for event link elements.
        static const String EventLinkTargetElement;     //!< Tag name for event link target elements.
        //! Tag name for specifying a source named area for a component
        static const String NamedAreaSourceElement;
        //! Tag name for specifying event / action responses for Child components
        static const String EventActionElement;
        // attribute names
        static const String VersionAttribute;           //!< Attribute name that stores the version number of the Falagard element containing the WidgetLookFeels.
        static const String TopLeftAttribute;           //!< Attribute name that stores colour for top-left corner.
        static const String TopRightAttribute;          //!< Attribute name that stores colour for top-right corner.
        static const String BottomLeftAttribute;        //!< Attribute name that stores colour for bottom-left corner.
        static const String BottomRightAttribute;       //!< Attribute name that stores colour for bottom-right corner.
        static const String TypeAttribute;              //!< Attribute name that stores a type string.
        static const String NameAttribute;              //!< Attribute name that stores name string
        static const String PriorityAttribute;          //!< Attribute name that stores an integer priority.
        static const String SectionNameAttribute;       //!< Attribute name that stores an imagery section name.
        static const String NameSuffixAttribute;        //!< Attribute name that stores a widget name suffix.
        static const String RendererAttribute;          //!< Attribute name that stores the name of a window renderer factory.
        static const String LookAttribute;              //!< Attribute name that stores the name of a widget look.
        static const String ScaleAttribute;             //!< Attribute name that stores a UDim scale value.
        static const String OffsetAttribute;            //!< Attribute name that stores a UDim offset value.
        static const String ValueAttribute;             //!< Attribute name that stores a property value string.
        static const String DimensionAttribute;         //!< Attribute name that stores a dimension type.
        static const String WidgetAttribute;            //!< Attribute name that stores the name of a widget (suffix).
        static const String StringAttribute;            //!< Attribute name that stores a string of text.
        static const String FontAttribute;              //!< Attribute name that stores the name of a font.
        static const String InitialValueAttribute;      //!< Attribute name that stores the initial default value for a property definition.
        static const String ClippedAttribute;           //!< Attribute name that stores whether some component will be clipped.
        static const String OperatorAttribute;          //!< Attribute name that stores the name of an operator.
        static const String PaddingAttribute;           //!< Attribute name that stores some padding value..
        static const String LayoutOnWriteAttribute;     //!< Attribute name that stores whether to layout on write of a property.
        static const String RedrawOnWriteAttribute;     //!< Attribute name that stores whether to redraw on write of a property.
        static const String TargetPropertyAttribute;    //!< Attribute name that stores a name of a target property.
        static const String ControlPropertyAttribute;   //!< Attribute name that stores a name of a property to control rendering of a section.
        static const String ColourAttribute;            //!< Attribute name that stores colour for all corners.
        static const String PropertyAttribute;          //!< Attribute name that stores the name of a property.
        static const String ControlValueAttribute;      //!< Attribute name that stores a test value to control rendering of a section.
        static const String ControlWidgetAttribute;     //!< Attribute name that stores a widget identifier used to control rendering of a section.
        //! Attribute name that stores a help string.
        static const String HelpStringAttribute;
        //! Attribute name that stores an Event name string.
        static const String EventAttribute;
        //! Attribute name that stores the name of an inherited WidgetLook
        static const String InheritsAttribute;
        //! Attribute name that stores auto-window preference
        static const String AutoWindowAttribute;
        //! Attribute name that stores name of event to fire for property defs
        static const String FireEventAttribute;
        //! Attribute name that stores the name of an action to be taken
        static const String ActionAttribute;
        //! Attribute name that stores some component enum value
        static const String ComponentAttribute;

    protected:
        /*************************************************************************
            ChainedXMLHandler base class overrides
        *************************************************************************/
        void elementStartLocal(const String& element,
                               const XMLAttributes& attributes);
        void elementEndLocal(const String& element);

    private:
        /*************************************************************************
            Typedefs
        *************************************************************************/
        //! Type for handlers of an opening xml element.
        typedef void (Falagard_xmlHandler::*ElementStartHandler)(const XMLAttributes& attributes);
        //! Type for handlers of a closing xml element.
        typedef void (Falagard_xmlHandler::*ElementEndHandler)();
        //! Map of handlers for opening xml elements.
        typedef std::map<String, ElementStartHandler, StringFastLessCompare> ElementStartHandlerMap;
        //! Map of handlers for closing xml elements.
        typedef std::map<String, ElementEndHandler, StringFastLessCompare> ElementEndHandlerMap;

        /*************************************************************************
            helper methods
        **************************************************************************/
        static argb_t hexStringToARGB(const String& str);

        /*************************************************************************
            implementation methods
        **************************************************************************/
        void assignAreaDimension(Dimension& dim);
        void assignColours(const ColourRect& colours);

        /*!
        \brief
            Method that performs common handling for all *Dim elements.
        */
        void doBaseDimStart(const BaseDim* dim);

        /*!
        \brief
            Method that handles the opening Falagard XML element.
        */
        void elementFalagardStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening WidgetLook XML element.
        */
        void elementWidgetLookStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening Child XML element.
        */
        void elementChildStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening ImagerySection XML element.
        */
        void elementImagerySectionStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening StateImagery XML element.
        */
        void elementStateImageryStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening Layer XML element.
        */
        void elementLayerStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening Section XML element.
        */
        void elementSectionStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening ImageryComponent XML element.
        */
        void elementImageryComponentStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening TextComponent XML element.
        */
        void elementTextComponentStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening FrameComponent XML element.
        */
        void elementFrameComponentStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening Area XML element.
        */
        void elementAreaStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening Image XML element.
        */
        void elementImageStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening Colours XML element.
        */
        void elementColoursStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening VertFormat XML element.
        */
        void elementVertFormatStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening HorzFormat XML element.
        */
        void elementHorzFormatStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening VertAlignment XML element.
        */
        void elementVertAlignmentStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening HorzAlignment XML element.
        */
        void elementHorzAlignmentStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening Property XML element.
        */
        void elementPropertyStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening Dim XML element.
        */
        void elementDimStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening UnifiedDim XML element.
        */
        void elementUnifiedDimStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening AbsoluteDim XML element.
        */
        void elementAbsoluteDimStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening ImageDim XML element.
        */
        void elementImageDimStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening ImagePropertyDim XML element.
        */
        void elementImagePropertyDimStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening WidgetDim XML element.
        */
        void elementWidgetDimStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening FontDim XML element.
        */
        void elementFontDimStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening PropertyDim XML element.
        */
        void elementPropertyDimStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening Text XML element.
        */
        void elementTextStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening ColourRectProperty XML element.
        */
        void elementColourRectPropertyStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening NamedArea XML element.
        */
        void elementNamedAreaStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening PropertyDefinition XML element.
        */
        void elementPropertyDefinitionStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening PropertyLinkDefinition XML element.
        */
        void elementPropertyLinkDefinitionStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening OperatorDim XML element.
        */
        void elementOperatorDimStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening VertFormatProperty XML element.
        */
        void elementVertFormatPropertyStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening HorzFormatProperty XML element.
        */
        void elementHorzFormatPropertyStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening AreaProperty XML element.
        */
        void elementAreaPropertyStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening ImageProperty XML element.
        */
        void elementImagePropertyStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening TextProperty XML element.
        */
        void elementTextPropertyStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening FontProperty XML element.
        */
        void elementFontPropertyStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the opening Colour XML element.
        */
        void elementColourStart(const XMLAttributes& attributes);

        //! Function to handle PropertyLinkTarget elements.
        void elementPropertyLinkTargetStart(const XMLAttributes& attributes);
        
        //! Function to handle AnimationDefinition elements
        void elementAnimationDefinitionStart(const XMLAttributes& attributes);

        //! Function to handle EventLinkDefinition elements.
        void elementEventLinkDefinitionStart(const XMLAttributes& attributes);

        //! Function to handle EventLinkTarget elements.
        void elementEventLinkTargetStart(const XMLAttributes& attributes);

        //! Function to handle NamedAreaSource elements.
        void elementNamedAreaSourceStart(const XMLAttributes& attributes);
        //! Function to handle EventAction elements.
        void elementEventActionStart(const XMLAttributes& attributes);

        /*!
        \brief
            Method that handles the closing Falagard XML element.
        */
        void elementFalagardEnd();

        /*!
        \brief
            Method that handles the closing WidgetLook XML element.
        */
        void elementWidgetLookEnd();

        /*!
        \brief
            Method that handles the closing Child XML element.
        */
        void elementChildEnd();

        /*!
        \brief
            Method that handles the closing ImagerySection XML element.
        */
        void elementImagerySectionEnd();

        /*!
        \brief
            Method that handles the closing StateImagery XML element.
        */
        void elementStateImageryEnd();

        /*!
        \brief
            Method that handles the closing Layer XML element.
        */
        void elementLayerEnd();

        /*!
        \brief
            Method that handles the closing Section XML element.
        */
        void elementSectionEnd();

        /*!
        \brief
            Method that handles the closing ImageryComponent XML element.
        */
        void elementImageryComponentEnd();

        /*!
        \brief
            Method that handles the closing TextComponent XML element.
        */
        void elementTextComponentEnd();

        /*!
        \brief
            Method that handles the closing FrameComponent XML element.
        */
        void elementFrameComponentEnd();

        /*!
        \brief
            Method that handles the closing Area XML element.
        */
        void elementAreaEnd();

        /*!
        \brief
            Method that handles the closing NamedArea XML element.
        */
        void elementNamedAreaEnd();

        /*!
        \brief
            Method that handles the closing XML for all *Dim elements.
        */
        void elementAnyDimEnd();

        //! Function to handle closing PropertyLinkDefinition XML element.
        void elementPropertyLinkDefinitionEnd();

        //! Function to handle closing EventLinkDefinition XML element.
        void elementEventLinkDefinitionEnd();

        /*!
        \brief
            Register a handler for the opening tag of an XML element
        */
        void registerElementStartHandler(const String& element, ElementStartHandler handler);

        /*!
        \brief
            Register a handler for the closing tag of an XML element
        */
        void registerElementEndHandler(const String& element, ElementEndHandler handler);
    
        //! helper to add an event link target as dictated by the input strings.
        void processEventLinkTarget(const String& widget, const String& event);

        /*************************************************************************
            Implementation Data
        *************************************************************************/
        WidgetLookManager* d_manager;

        // these are used to implement the handler without using a huge
        // if / else if /else construct, we just register the element name, and
        // handler member function, and everything else is done using those
        // mappings.
        ElementStartHandlerMap  d_startHandlersMap;
        ElementEndHandlerMap    d_endHandlersMap;

        // these hold pointers to various objects under construction.
        WidgetLookFeel*     d_widgetlook;
        WidgetComponent*    d_childcomponent;
        ImagerySection*     d_imagerysection;
        StateImagery*       d_stateimagery;
        LayerSpecification* d_layer;
        SectionSpecification* d_section;
        ImageryComponent*   d_imagerycomponent;
        ComponentArea*   d_area;
        Dimension       d_dimension;
        TextComponent*  d_textcomponent;
        NamedArea*      d_namedArea;
        FrameComponent*  d_framecomponent;

        std::vector<BaseDim*
            CEGUI_VECTOR_ALLOC(BaseDim*)> d_dimStack;

        PropertyDefinitionBase* d_propertyLink;
        EventLinkDefinition* d_eventLink;
    };

} // End of  CEGUI namespace section


#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif  // end of guard _CEGUIFalagard_xmlHandler_h_

