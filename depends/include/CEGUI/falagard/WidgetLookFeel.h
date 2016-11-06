/***********************************************************************
    created:    Mon Jun 13 2005
    author:     Paul D Turner <paul@cegui.org.uk>
                edited by Lukas E Meindl in Jul 2014
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2010 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUIFalWidgetLookFeel_h_
#define _CEGUIFalWidgetLookFeel_h_

#include "./StateImagery.h"
#include "./WidgetComponent.h"
#include "./ImagerySection.h"
#include "./PropertyInitialiser.h"
#include "./PropertyDefinition.h"
#include "./PropertyLinkDefinition.h"
#include "./EventLinkDefinition.h"
#include "./NamedArea.h"
#include "./NamedDefinitionCollator.h"
#include <map>

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif


namespace CEGUI
{
/*!
\brief
    Class that encapsulates Look N' Feel information for a widget.
*/
class CEGUIEXPORT WidgetLookFeel :
    public AllocatedObject<WidgetLookFeel>
{
public:
    WidgetLookFeel(const String& name, const String& inherits);
    WidgetLookFeel() {}
    WidgetLookFeel(const WidgetLookFeel& other);
    WidgetLookFeel& operator=(const WidgetLookFeel& other);

    virtual ~WidgetLookFeel();
    /*!
    \brief
        Return a const reference to the StateImagery object for the specified
        state.

    \return
        StateImagery object for the requested state.
    */
    const StateImagery& getStateImagery(const CEGUI::String& state) const;

    /*!
    \brief
        Return a const reference to the ImagerySection object with the
        specified name.

    \return
        ImagerySection object with the specified name.
    */
    const ImagerySection& getImagerySection(const CEGUI::String& section) const;

    /*!
    \brief
        Return the name of the widget look.

    \return
        String object holding the name of the WidgetLookFeel.
    */
    const String& getName() const;

    /*!
    \brief
        Add an ImagerySection to the WidgetLookFeel.

    \param section
        ImagerySection object to be added.

    \return
        Nothing.
    */
    void addImagerySection(const ImagerySection& section);

    //! \deprecated This function is to be replaced by a new renameImagerySection function in the new version, which considers inheritance and accepts more appropriate parameters.
    void renameImagerySection(const String& oldName, const String& newName);

    /*!
    \brief
        Add a WidgetComponent to the WidgetLookFeel.

    \param widget
        WidgetComponent object to be added.

    \return
        Nothing.
    */
    void addWidgetComponent(const WidgetComponent& widget);

    /*!
    \brief
        Add a state specification (StateImagery object) to the WidgetLookFeel.

    \param section
        StateImagery object to be added.

    \return
        Nothing.
    */
    void addStateSpecification(const StateImagery& state);

    /*!
    \brief
        Add a property initialiser to the WidgetLookFeel.

    \param initialiser
        PropertyInitialiser object to be added.

    \return
        Nothing.
    */
    void addPropertyInitialiser(const PropertyInitialiser& initialiser);

    /*!
    \brief
        Clear all ImagerySections from the WidgetLookFeel.

    \return
        Nothing.
    */
    void clearImagerySections();

    /*!
    \brief
        Clear all WidgetComponents from the WidgetLookFeel.

    \return
        Nothing.
    */
    void clearWidgetComponents();

    /*!
    \brief
        Clear all StateImagery objects from the WidgetLookFeel.

    \return
        Nothing.
    */
    void clearStateSpecifications();

    /*!
    \brief
        Clear all PropertyInitialiser objects from the WidgetLookFeel.

    \return
        Nothing.
    */
    void clearPropertyInitialisers();

    /*!
    \brief
        Initialise the given window using PropertyInitialsers and component
        widgets specified for this WidgetLookFeel.

    \param widget
        Window based object to be initialised.

    \return
        Nothing.
    */
    void initialiseWidget(Window& widget) const;

    /*!
    \brief
        Clean up the given window from all properties and component widgets
        created by this WidgetLookFeel

    \param widget
        Window based object to be cleaned up.

    \return
        Nothing.
    */
    void cleanUpWidget(Window& widget) const;

    /*!
    \brief
        Return whether imagery is defined for the given state.

    \param state
        String object containing name of state to look for.

    \return
        - true if imagery exists for the specified state,
        - false if no imagery exists for the specified state.

    \deprecated This function is deprecated because it does not consider WidgetLook inheritance. It will be replaced by a
                new isStateImageryPresent function in the next major version.
    */
    bool isStateImageryPresent(const String& state) const;

    /*!
    \brief
        Adds a named area to the WidgetLookFeel.

    \param area
        NamedArea to be added.

    \return
        Nothing.
    */
    void addNamedArea(const NamedArea& area);

    /*!
    \brief
        Clear all defined named areas from the WidgetLookFeel

    \return
        Nothing.
    */
    void clearNamedAreas();

    /*!
    \brief
        Return the NamedArea with the specified name.

    \param name
        String object holding the name of the NamedArea to be returned.

    \return
        The requested NamedArea object.
    */
    const NamedArea& getNamedArea(const String& name) const;

    //! \deprecated This function is to be replaced by a new renameNamedArea function in the new version, which considers inheritance and accepts more appropriate parameters.
    void renameNamedArea(const String& oldName, const String& newName);

    /*!
    \brief
        return whether a NamedArea object with the specified name exists for
        this WidgetLookFeel.

    \param name
        String holding the name of the NamedArea to check for.

    \return
        - true if a named area with the requested name is defined for this
          WidgetLookFeel.
        - false if no such named area is defined for this WidgetLookFeel.

    \deprecated This function is deprecated because it does not consider WidgetLook inheritance. It will be changed to isNamedAreaPresent() in the next major version.
    */
    bool isNamedAreaDefined(const String& name) const;

    /*!
    \brief
        Layout the child widgets defined for this WidgetLookFeel which are
        attached to the given window.

    \param owner
        Window object that has the child widgets that require laying out.

    \return
        Nothing.
    */
    void layoutChildWidgets(const Window& owner) const;

    /*!
    \brief
        Adds a property definition to the WidgetLookFeel.

    \param propdef
        PropertyDefinition object to be added.

    \return
        Nothing.
    */
    void addPropertyDefinition(PropertyDefinitionBase* propdef);

    /*!
    \brief
        Adds a property link definition to the WidgetLookFeel.

    \param propdef
        PropertyLinkDefinition object to be added.

    \return
        Nothing.
    */
    void addPropertyLinkDefinition(PropertyDefinitionBase* propdef);

    /*!
    \brief
        Clear all defined property definitions from the WidgetLookFeel

    \return
        Nothing.
    */
    void clearPropertyDefinitions();

    /*!
    \brief
        Clear all defined property link definitions from the WidgetLookFeel

    \return
        Nothing.
    */
    void clearPropertyLinkDefinitions();

    /*!
    \brief
        Add the name of an animation that is associated with the
        WidgetLookFeel.

    \param anim_name
        Reference to a String object that contains the name of the animation
        to be associated with this WidgetLookFeel.
    */
    void addAnimationName(const String& anim_name);

    //! adds an event link definition to the WidgetLookFeel.
    void addEventLinkDefinition(const EventLinkDefinition& evtdef); 

    //! clear all defined event link definitions from the WidgetLookFeel.
    void clearEventLinkDefinitions();

    /*!
    \brief
        Writes an xml representation of this WidgetLookFeel to \a out_stream.

    \param xml_stream
        Stream where xml data should be output.

    \return
        Nothing.
    */
    void writeXMLToStream(XMLSerializer& xml_stream) const;

    /*!
    \brief
        Takes the name of a property and returns a pointer to the last
        PropertyInitialiser for this property or 0 if the is no
        PropertyInitialiser for this property in the WidgetLookFeel

    \param propertyName
        The name of the property to look for.
    */
    const PropertyInitialiser* findPropertyInitialiser(const String& propertyName) const;

    /*!
    \brief
        Takes the name for a WidgetComponent and returns a pointer to
        it if it exists or a null pointer if it doesn't.

    \param name
        The name of the Child component to look for.

    \deprecated
        This function will be replaced by getWidgetComponent in the next version.
    */
    const WidgetComponent* findWidgetComponent(const String& name) const;


    /*!
    \brief
        Takes the name for a WidgetComponent and returns a pointer to
        it if it exists or null pointer if it doesn't.

    \param name
        The name of the WidgetComponent to look for.

    \param includeInheritedElements
        If set to true, this function will try to also include elements from the inherited WidgetLookFeel.

    \return
        A pointer to the WidgetComponent.

    \deprecated
        This function will be replaced by getWidgetComponent in the next version.
    */
    WidgetComponent* retrieveWidgetComponentFromList(const String& name, bool includeInheritedElements = false);

    /*!
    \brief
        Takes the name for a Property (PropertyInitialiser) and returns a pointer to
        it if it exists or 0 if it doesn't.

    \param name
        The name of the Property (PropertyInitialiser) to look for.

    \param includeInheritedElements
        If set to true, this function will try to also include elements from the inherited WidgetLookFeel.

    \return
        A pointer to the Property (PropertyInitialiser).

    \deprecated
        This function will be replaced by getPropertyInitialiser in the next version.
    */
    PropertyInitialiser* retrievePropertyInitialiserFromList(const String& name, bool includeInheritedElements = false);

    /*!
    \brief
        Takes the name for a PropertyDefinition and returns a pointer to
        it if it exists or 0 if it doesn't.

    \param name
        The name of the PropertyDefinition (PropertyDefinitionBase) to look for.

    \param includeInheritedElements
        If set to true, this function will try to also include elements from the inherited WidgetLookFeel.

    \return
        A pointer to the PropertyDefinition (PropertyDefinitionBase).

    \deprecated
        This function will be replaced by getPropertyDefinition in the next version.
    */
    PropertyDefinitionBase* retrievePropertyDefinitionFromList(const String& name, bool includeInheritedElements = false);

    /*!
    \brief
        Takes the name for a PropertyLinkDefinition and returns a pointer to
        it if it exists or 0 if it doesn't.

    \param name
        The name of the PropertyLinkDefinition (PropertyDefinitionBase) to look for.

    \param includeInheritedElements
        If set to true, this function will try to also include elements from the inherited WidgetLookFeel.

    \return
        A pointer to the PropertyLinkDefinition (PropertyDefinitionBase).

    \deprecated
        This function will be replaced by getPropertyLinkDefinition in the next version.
    */
    PropertyDefinitionBase* retrievePropertyLinkDefinitionFromList(const String& name, bool includeInheritedElements = false);

    /*!
    \brief
        Takes the name for a EventLinkDefinition and returns a pointer to
        it if it exists or 0 if it doesn't.

    \param name
        The name of the EventLinkDefinition to look for.

    \param includeInheritedElements
        If set to true, this function will try to also include elements from the inherited WidgetLookFeel.

    \return
        A pointer to the EventLinkDefinition.

    \deprecated
        This function will be replaced by getEventLinkDefinition in the next version.
    */
    EventLinkDefinition* retrieveEventLinkDefinitionFromList(const String& name, bool includeInheritedElements = false);
    

    typedef std::set<String, StringFastLessCompare
        CEGUI_SET_ALLOC(String)> StringSet;

    /** Typedefs for maps of Falagard elements this WidgetLookFeel owns. */
    //! Map of Strings to StateImagery pointers
    typedef std::map<String, StateImagery*, StringFastLessCompare
        CEGUI_MAP_ALLOC(String, StateImagery*)> StateImageryPointerMap;
    //! Map of Strings to ImagerySection pointers
    typedef std::map<String, ImagerySection*, StringFastLessCompare
        CEGUI_MAP_ALLOC(String, ImagerySection*)> ImagerySectionPointerMap;
    //! Map of Strings to NamedArea pointers
    typedef std::map<String, NamedArea*, StringFastLessCompare
        CEGUI_MAP_ALLOC(String, NamedArea*)> NamedAreaPointerMap;
    //! Map of Strings to WidgetComponent pointers
    typedef std::map<String, WidgetComponent*, StringFastLessCompare
        CEGUI_MAP_ALLOC(String, WidgetComponent*)> WidgetComponentPointerMap;
    //! Map of Strings to AnimationInstance pointers
    typedef std::map<String, AnimationInstance*, StringFastLessCompare
        CEGUI_MAP_ALLOC(String, AnimationInstance*)> AnimationInstancePointerMap;
    //! Map of Strings to EventLinkDefinition pointers
    typedef std::map<String, EventLinkDefinition*, StringFastLessCompare
        CEGUI_MAP_ALLOC(String, EventLinkDefinition*)> EventLinkDefinitionPointerMap;

    /** Typedefs for maps of property related elements this WidgetLookFeel owns. */
    //! Map of Strings to PropertyInitialiser pointers
    typedef std::map<String, PropertyInitialiser*, StringFastLessCompare
        CEGUI_MAP_ALLOC(String, PropertyInitialiser*)> PropertyInitialiserPointerMap;
    //! Map of Strings to PropertyDefinitionBase pointers
    typedef std::map<String, PropertyDefinitionBase*, StringFastLessCompare
        CEGUI_MAP_ALLOC(String, PropertyDefinitionBase*)> PropertyDefinitionBasePointerMap;



    /** Typedefs for property related lists. */
    //! \deprecated This type is deprecated because it will be moved into the private section and changed into a map in the next version.
    typedef std::vector<PropertyInitialiser
        CEGUI_VECTOR_ALLOC(PropertyInitialiser)> PropertyList;
    //! \deprecated This type is deprecated because it will be moved into the private section and changed into a map in the next version.
    typedef std::vector<PropertyDefinitionBase*
        CEGUI_VECTOR_ALLOC(PropertyDefinitionBase*)> PropertyDefinitionList;
    //! \deprecated This type is deprecated because it will be moved into the private section and changed into a map in the next version.
    typedef std::vector<PropertyDefinitionBase*
        CEGUI_VECTOR_ALLOC(PropertyDefinitionBase*)> PropertyLinkDefinitionList;



    /** Obtains list of properties definitions.
     * @access public
     * @return CEGUI::WidgetLookFeel::PropertyDefinitionList List of properties
     * definitions
     */
    /*!
        \deprecated
            This function is deprecated because the return type is to be replaced by a map, a bool parameter for WLF-inheritance added.
            The function will be replaced by getPropertyDefinitionMap in the next version.
    */
    const PropertyDefinitionList& getPropertyDefinitions() const
    {
        return d_propertyDefinitions;
    }

    /** Obtains list of properties link definitions.
     * @access public
     * @return CEGUI::WidgetLookFeel::PropertyLinkDefinitionList List of
     * properties link definitions
     */
    /*!
        \deprecated
            This function is deprecated because the return type is to be replaced by a map, a bool parameter for WLF-inheritance added.
            The function will be replaced by getPropertyLinkDefinitionMap in the next version.
    */
    const PropertyLinkDefinitionList& getPropertyLinkDefinitions() const
    {
        return d_propertyLinkDefinitions;
    }

    /** Obtains list of properties.
     * @access public
     * @return CEGUI::WidgetLookFeel::PropertyList List of properties
     */
    /*!
        \deprecated
            This function is deprecated because the return type is to be replaced by a map, a bool parameter for WLF-inheritance added.
            The function will be replaced by getPropertyMap in the next version.
    */
    const PropertyList& getProperties() const
    {
        return d_properties;
    }

    //! perform any processing required due to the given font having changed.
    bool handleFontRenderSizeChange(Window& window, const Font* font) const;



    /*!
    \brief
        Returns a map of names to pointers for all StateImagery elements this WidgetLookFeel owns. If the list of
        StateImageries of this or an inherited WidgetLookFeel is modified in any way (by clearing it, adding or
        removing elements), then the pointers in this map are not valid anymore. The function should then be called
        again to retrieve valid pointers.

    \param includeInheritedElements
        If set to true, this function will try to also include elements from the inherited WidgetLookFeel.

    \return
        A map of names to StateImagery pointers.
    */
    StateImageryPointerMap getStateImageryMap(bool includeInheritedElements = false);
    
    /*!
    \brief
        Returns a map of names to pointers for all ImagerySection elements this WidgetLookFeel owns. If the list of
        ImagerySections of this or an inherited WidgetLookFeel is modified in any way (by clearing it, adding or
        removing elements), then the pointers in this map are not valid anymore. The function should then be called
        again to retrieve valid pointers.

    \param includeInheritedElements
        If set to true, this function will try to also include elements from the inherited WidgetLookFeel.

    \return
        A map of names to ImagerySection pointers.
    */
    ImagerySectionPointerMap getImagerySectionMap(bool includeInheritedElements = false);

    /*!
    \brief
        Returns a map of names to pointers for all NamedArea elements this WidgetLookFeel owns. If the list of
        NamedAreas of this or an inherited WidgetLookFeel is modified in any way (by clearing it, adding or
        removing elements), then the pointers in this map are not valid anymore. The function should then be called
        again to retrieve valid pointers.

    \param includeInheritedElements
        If set to true, this function will try to also include elements from the inherited WidgetLookFeel.

    \return
        A map of names to NamedArea pointers.
    */
    NamedAreaPointerMap getNamedAreaMap(bool includeInheritedElements = false);

    /*!
    \brief
        Returns a map of names to pointers for all WidgetComponent elements this WidgetLookFeel owns. If the list of
        WidgetComponents of this or an inherited WidgetLookFeel is modified in any way (by clearing it, adding or
        removing elements), then the pointers in this map are not valid anymore. The function should then be called
        again to retrieve valid pointers.

    \param includeInheritedElements
        If set to true, this function will try to also include elements from the inherited WidgetLookFeel.

    \return
        A map of names to WidgetComponent pointers.
    */
    WidgetComponentPointerMap getWidgetComponentMap(bool includeInheritedElements = false);
  
    /*!
    \brief
        Returns a map of names to pointers for all PropertyInitialiser elements this WidgetLookFeel owns. If the list of
        PropertyInitialisers of this or an inherited WidgetLookFeel is modified in any way (by clearing it, adding or
        removing elements), then the pointers in this map are not valid anymore. The function should then be called
        again to retrieve valid pointers.

    \param includeInheritedElements
        If set to true, this function will try to also include elements from the inherited WidgetLookFeel.

    \return
        A map of names to PropertyInitialiser pointers.
    */
    PropertyInitialiserPointerMap getPropertyInitialiserMap(bool includeInheritedElements = false);

    /*!
    \brief
        Returns a map of names to pointers for all PropertyDefinition elements this WidgetLookFeel owns. If the list of
        PropertyDefinitions of this or an inherited WidgetLookFeel is modified in any way (by clearing it, adding or
        removing elements), then the pointers in this map are not valid anymore. The function should then be called
        again to retrieve valid pointers.

    \param includeInheritedElements
        If set to true, this function will try to also include elements from the inherited WidgetLookFeel.

    \return
        A map of names to PropertyDefinition pointers.
    */
    PropertyDefinitionBasePointerMap getPropertyDefinitionMap(bool includeInheritedElements = false);

    /*!
    \brief
        Returns a map of names to pointers for all PropertyLinkDefinition elements this WidgetLookFeel owns. If the list of
        PropertyLinkDefinitions of this or an inherited WidgetLookFeel is modified in any way (by clearing it, adding or
        removing elements), then the pointers in this map are not valid anymore. The function should then be called
        again to retrieve valid pointers.

    \param includeInheritedElements
        If set to true, this function will try to also include elements from the inherited WidgetLookFeel.

    \return
        A map of names to PropertyLinkDefinition pointers.
    */
    PropertyDefinitionBasePointerMap getPropertyLinkDefinitionMap(bool includeInheritedElements = false);

    /*!
    \brief
        Returns a map of names to pointers for all EventLinkDefinition elements this WidgetLookFeel owns. If the list of
        EventLinkDefinitions of this or an inherited WidgetLookFeel is modified in any way (by clearing it, adding or
        removing elements), then the pointers in this map are not valid anymore. The function should then be called
        again to retrieve valid pointers.

    \param includeInheritedElements
        If set to true, this function will try to also include elements from the inherited WidgetLookFeel.

    \return
        A map of names to EventLinkDefinition pointers.
    */
    EventLinkDefinitionPointerMap getEventLinkDefinitionMap(bool includeInheritedElements = false);

    /*!
    \brief
        Returns a String containing the name of the inherited WidgetLookFeel.

    \return
        A String containing the name of the inherited WidgetLookFeel.
    */
    const CEGUI::String& getInheritedWidgetLookName() const;

    StringSet getStateImageryNames(bool inherits = false) const;
    //! \deprecated This function has been renamed to getStateImageryNames to conform the general way of naming functions, and will be replaced in the next version
    StringSet getStateNames(bool inherits = false) const;
    StringSet getImagerySectionNames(bool inherits = false) const;
    //! \deprecated This function has been renamed to getImagerySectionNames to conform the general way of naming functions, and will be replaced in the next version
    StringSet getImageryNames(bool inherits = false) const;
    StringSet getNamedAreaNames(bool inherits = false) const;
    StringSet getWidgetComponentNames(bool inherits = false) const;
    //! \deprecated This function has been renamed to getWidgetComponentNames to conform the general way of naming functions, and will be replaced in the next version
    StringSet getWidgetNames(bool inherits = false) const;
    StringSet getPropertyInitialiserNames(bool inherits = false) const;
    StringSet getPropertyDefinitionNames(bool inherits = false) const;
    StringSet getPropertyLinkDefinitionNames(bool inherits = false) const;
    StringSet getEventLinkDefinitionNames(bool inherits = false) const;
    StringSet getAnimationNames(bool inherits = false) const;

private:

    /*!
    \brief
        Returns a pointer to the inherited WidgetLookFeel. If this WidgetLookFeel does not inherit from another, the function returns 0.

    \return
        A pointer to the inherited WidgetLookFeel.
    */
    WidgetLookFeel* getInheritedWidgetLookFeel();


    typedef std::map<String, StateImagery, StringFastLessCompare
        CEGUI_MAP_ALLOC(String, StateImagery)> StateList;
    typedef std::map<String, ImagerySection, StringFastLessCompare
        CEGUI_MAP_ALLOC(String, ImagerySection)> ImageryList;
    typedef std::map<String, NamedArea, StringFastLessCompare
        CEGUI_MAP_ALLOC(String, NamedArea)> NamedAreaList;

    //! \deprecated The type is deprecated, as it will be changed to a map in the next CEGUI version, in accordance with the existing StateImagery/ImagerySection/NamedArea container types
    typedef std::vector<WidgetComponent
        CEGUI_VECTOR_ALLOC(WidgetComponent)> WidgetList;

    typedef std::vector<String
        CEGUI_VECTOR_ALLOC(String)> AnimationList;
    //! The type is deprecated, as it will be changed to have the correct CEGUI allocator used starting in the next version
    typedef std::multimap<Window*, AnimationInstance*
        /*CEGUI_MULTIMAP_ALLOC(Window*, AnimationInstance*)*/> AnimationInstanceMap;

    //! \deprecated The type is deprecated, as it will be changed to a map in the next CEGUI version, in accordance with the existing StateImagery/ImagerySection/NamedArea container types
    typedef std::vector<EventLinkDefinition
        CEGUI_VECTOR_ALLOC(EventLinkDefinition)> EventLinkDefinitionList;

    //! Name of this WidgetLookFeel.
    CEGUI::String d_lookName;
    //! Name of a WidgetLookFeel inherited by this WidgetLookFeel.
    CEGUI::String d_inheritedLookName;
    //! Collection of ImagerySection objects.
    ImageryList d_imagerySections;
    //! Collection of WidgetComponent objects.
    WidgetList d_childWidgets;
    //! Collection of StateImagery objects.
    StateList d_stateImagery;
    //! Collection of PropertyInitialser objects.
    PropertyList d_properties;
    //! Collection of NamedArea objects.
    NamedAreaList d_namedAreas;
    //! Collection of PropertyDefinition objects.
    mutable PropertyDefinitionList  d_propertyDefinitions;
    //! Collection of PropertyLinkDefinition objects.
    mutable PropertyLinkDefinitionList d_propertyLinkDefinitions;
    //! Collection of animation names associated with this WidgetLookFeel.
    AnimationList d_animations;
    //! map of windows and their associated animation instances
    mutable AnimationInstanceMap d_animationInstances;
    //! Collection of EventLinkDefinition objects.
    EventLinkDefinitionList d_eventLinkDefinitions;

    // these are container types used when composing final collections of
    // objects that come via inheritence.
    typedef NamedDefinitionCollator<String, const WidgetComponent*> WidgetComponentCollator;
    typedef NamedDefinitionCollator<String, PropertyDefinitionBase*> PropertyDefinitionCollator;
    typedef NamedDefinitionCollator<String, PropertyDefinitionBase*> PropertyLinkDefinitionCollator;
    typedef NamedDefinitionCollator<String, const PropertyInitialiser*> PropertyInitialiserCollator;
    typedef NamedDefinitionCollator<String, const EventLinkDefinition*> EventLinkDefinitionCollator;
    typedef std::set<String, StringFastLessCompare
        CEGUI_SET_ALLOC(String)> AnimationNameSet;

    // functions to populate containers with collections of objects that we
    // gain through inheritence.
    void appendChildWidgetComponents(WidgetComponentCollator& col, bool inherits = true) const;
    void appendPropertyDefinitions(PropertyDefinitionCollator& col, bool inherits = true) const;
    void appendPropertyLinkDefinitions(PropertyLinkDefinitionCollator& col, bool inherits = true) const;
    void appendPropertyInitialisers(PropertyInitialiserCollator& col, bool inherits = true) const;
    void appendEventLinkDefinitions(EventLinkDefinitionCollator& col, bool inherits = true) const;
    void appendAnimationNames(AnimationNameSet& set, bool inherits = true) const;

    void swap(WidgetLookFeel& other);

public:
    /*************************************************************************
        Iterator stuff
    *************************************************************************/

    //! \deprecated This typedef is deprecated because all iterator getter functions will be removed. Please use the getStateImageryMap function instead to access the container.
    typedef ConstMapIterator<StateList> StateIterator;
    //! \deprecated This typedef is deprecated because all iterator getter functions will be removed. Please use the getImagerySectionMap function instead to access the container.
    typedef ConstMapIterator<ImageryList> ImageryIterator;
    //! \deprecated This typedef is deprecated because all iterator getter functions will be removed. Please use the getNamedAreaMap function instead to access the container.
    typedef ConstMapIterator<NamedAreaList> NamedAreaIterator;

    //! \deprecated This typedef is deprecated because all iterator getter functions will be removed. Please use the getWidgetComponentMap function instead to access the container.
    typedef ConstVectorIterator<WidgetComponentCollator> WidgetComponentIterator;
    //! \deprecated This typedef is deprecated because all iterator getter functions will be removed. Please use the getPropertyDefinitionMap function instead to access the container.
    typedef ConstVectorIterator<PropertyDefinitionCollator> PropertyDefinitionIterator;
    //! \deprecated This typedef is deprecated because all iterator getter functions will be removed. Please use the getPropertyLinkDefinitionMap function instead to access the container.
    typedef ConstVectorIterator<PropertyLinkDefinitionCollator> PropertyLinkDefinitionIterator;
    //! \deprecated This typedef is deprecated because all iterator getter functions will be removed. Please use the getPropertyInitialiserMap function instead to access the container.
    typedef ConstVectorIterator<PropertyInitialiserCollator> PropertyInitialiserIterator;
    //! \deprecated This typedef is deprecated because all iterator getter functions will be removed. Please use the getEventLinkDefinitionMap function instead to access the container.
    typedef ConstVectorIterator<EventLinkDefinitionCollator> EventLinkDefinitionIterator;
    //! \deprecated This typedef is deprecated because all iterator getter functions will be removed. Please use the getAnimationNames function instead to access the container.
    typedef ConstVectorIterator<AnimationNameSet> AnimationNameIterator;

    /*!
    \brief
        Returns an iterator for the StateImageries of this WidgetLookFeel.

    \deprecated
        This function is deprecated because all iterator getter functions will be removed. Please use the getStateImageryMap function instead to access the container.
    */
    StateIterator getStateIterator(bool inherits = false) const;
    /*!
    \brief
        Returns an iterator for the ImagerySections of this WidgetLookFeel.

    \deprecated
        This function is deprecated because all iterator getter functions will be removed. Please use the getImagerySectionMap function instead to access the container.
    */
    ImageryIterator getImageryIterator(bool inherits = false) const;

    /*!
    \brief
        Returns an iterator for the NamedAreas of this WidgetLookFeel.

    \deprecated
        This function is deprecated because all iterator getter functions will be removed. Please use the getNamedAreaMap function instead to access the container.
    */
    NamedAreaIterator getNamedAreaIterator(bool inherits = false) const;

    /*!
    \brief
        Returns an iterator for the WidgetComponents of this WidgetLookFeel.

    \deprecated
        This function is deprecated because all iterator getter functions will be removed. Please use the getWidgetComponentMap function instead to access the container.
    */
    WidgetComponentIterator getWidgetComponentIterator(bool inherits = false) const;

    /*!
    \brief
        Returns an iterator for the PropertyDefinitions of this WidgetLookFeel.

    \deprecated
        This function is deprecated because all iterator getter functions will be removed. Please use the getPropertyDefinitionMap function instead to access the container.
    */
    PropertyDefinitionIterator getPropertyDefinitionIterator(bool inherits = false) const;

    /*!
    \brief
        Returns an iterator for the PropertyLinkDefinitions of this WidgetLookFeel.

    \deprecated
        This function is deprecated because all iterator getter functions will be removed. Please use the getPropertyLinkDefinitionMap function instead to access the container.
    */
    PropertyLinkDefinitionIterator getPropertyLinkDefinitionIterator(bool inherits = false) const;

    /*!
    \brief
        Returns an iterator for the PropertyInitialisers of this WidgetLookFeel.

    \deprecated
        This function is deprecated because all iterator getter functions will be removed. Please use the getPropertyInitialiserMap function instead to access the container.
    */
    PropertyInitialiserIterator getPropertyInitialiserIterator(bool inherits = false) const;

    /*!
    \brief
        Returns an iterator for the EventLinkDefinitions of this WidgetLookFeel.

    \deprecated
        This function is deprecated because all iterator getter functions will be removed. Please use the getEventLinkDefinitionMap function instead to access the container.
    */
    EventLinkDefinitionIterator getEventLinkDefinitionIterator(bool inherits = false) const;

    /*!
    \brief
        Returns an iterator for the AnimationNames of this WidgetLookFeel.

    \deprecated
        This function is deprecated because all iterator getter functions will be removed. Please use the getAnimationNames function instead to access the container.
    */
    AnimationNameIterator getAnimationNameIterator(bool inherits = false) const;
};

}


#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif

