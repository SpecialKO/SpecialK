/***********************************************************************
	created:	21/2/2004
	author:		Paul D Turner
	
	purpose:	Defines interface for the PropertySet class
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
#ifndef _CEGUIPropertySet_h_
#define _CEGUIPropertySet_h_

#include "CEGUI/Base.h"
#include "CEGUI/String.h"
#include "CEGUI/IteratorBase.h"
#include "CEGUI/Property.h"
#include "CEGUI/PropertyHelper.h"
#include "CEGUI/TypedProperty.h"
// not needed in this header but you are likely to use it if you include this,
// we also define the CEGUI_DEFINE_PROPERTY macro that relies on this here
#include "CEGUI/TplWindowProperty.h"
#include "CEGUI/Exceptions.h"
#include <map>

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
	Interface providing introspection capabilities

CEGUI uses this interface for introspection and serialisation, especially in
CEGUI::Window and classes that inherit it.

If you are just a user of an existing PropertySet class there are just 2 methods
that you should be aware of - PropertySet::setProperty and PropertySet::getProperty.
Both methods are available in 2 variants - string fallback mode and templated
native mode.  It is recommended to use the native mode. Code example:

\code{.cpp}
CEGUI::Window* wnd = ...;
// native mode property set
wnd->setProperty<float>("Alpha", 0.5f); // set Alpha to 50%
// string fallback mode
wnd->setProperty("Alpha", "0.5"); // set Alpha to 50%

// native mode property get
const float alpha = wnd->getProperty<float>("Alpha");
// string fallback mode
const String alphaString = wnd->getProperty("Alpha");
\endcode

If you use native mode with the wrong type CEGUI will resort to string fallback
and will try to convert the type to whatever you requested.

\code{.cpp}
CEGUI::Window* wnd = ...;
// will set Alpha to 100% but is slower due to the casts
wnd->setProperty<int>("Alpha", 1);
// retrieves Alpha as string, then proceeds to convert that to int
// beware of rounding errors!
wnd->getProperty<int>("Alpha");
\endcode

We will always offer string fallback mode because it is necessary for serialisation
and scripting. Scripting languages are often duck typed and cannot use C++
templated code.

The CEGUI::Property instances hold pointers to getter and setter of
a given property. They are designed in such a way that multiple PropertySets
(multiple widgets) can share the Property instances to save memory. This means
that PropertySets don't own the Properties themselves. The Property instances
are usually static members of the PropertySet classes or static local
variables in the scope of the constructor or a method called from there.

It's unusual but multiple instances of the same class can have different
Properties added to them.

It is recommended to use the \a CEGUI_DEFINE_PROPERTY macro instead of using
PropertySet::addProperty directly. This takes care of property initialisation
as well as it's addition to the PropertySet instance.
*/
class CEGUIEXPORT PropertySet : public PropertyReceiver
{
public:
    /*!
	\brief
		Constructs a new PropertySet object
	*/
    PropertySet(void) {}


    /*!
	\brief
		Destructor for PropertySet objects.
	*/
    virtual ~PropertySet(void) {}


    /*!
	\brief
		Adds a new Property to the PropertySet

	\param property
		Pointer to the Property object to be added to the PropertySet.

	\exception NullObjectException		Thrown if \a property is NULL.
	\exception AlreadyExistsException	Thrown if a Property with the same name as \a property already exists in the PropertySet

    \note
        You most likely don't want to use this method unless you are implementing
        a custom Property class. Please see CEGUI_DEFINE_PROPERTY instead.
	*/
    void addProperty(Property* property);


    /*!
	\brief
		Removes a Property from the PropertySet.

	\param name
		String containing the name of the Property to be removed.  If Property \a name is not in the set, nothing happens.
	*/
    void removeProperty(const String& name);


    /*!
    \brief
        Retrieves a property instance (that was previously added)

    \param name
        String containing the name of the Property to be retrieved. If Property \a name is not in the set, exception is thrown.

    \return
        Pointer to the property instance
    */
    Property* getPropertyInstance(const String& name) const;


    /*!
	\brief
		Removes all Property objects from the PropertySet.
    */
    void clearProperties(void);


    /*!
	\brief
		Checks to see if a Property with the given name is in the PropertySet

	\param name
		String containing the name of the Property to check for.

	\return
		true if a Property named \a name is in the PropertySet.  false if no Property named \a name is in the PropertySet.
	*/
    bool isPropertyPresent(const String& name) const;


    /*!
	\brief
		Return the help text for the specified Property.

	\param name
		String holding the name of the Property who's help text is to be returned.

	\return
		String object containing the help text for the Property \a name.

	\exception UnknownObjectException	Thrown if no Property named \a name is in the PropertySet.
	*/
    const String& getPropertyHelp(const String& name) const;


    /*!
	\brief
		Gets the current value of the specified Property.

	\param name
		String containing the name of the Property who's value is to be returned.

	\return
		String object containing a textual representation of the requested Property.

	\exception UnknownObjectException	Thrown if no Property named \a name is in the PropertySet.
	*/
    String getProperty(const String& name) const;

    /*!
    \copydoc PropertySet::getProperty
    
    This method tries to do a native type get without string conversion if possible,
    if that is not possible, it gracefully falls back to string conversion
    */
    template<typename T>
    typename PropertyHelper<T>::return_type getProperty(const String& name) const
    {
        PropertyRegistry::const_iterator pos = d_properties.find(name);

        if (pos == d_properties.end())
        {
            CEGUI_THROW(UnknownObjectException("There is no Property named '" + name + "' available in the set."));
        }

        Property* baseProperty = pos->second;
        TypedProperty<T>* typedProperty = dynamic_cast<TypedProperty<T>* >(baseProperty);

        if (typedProperty)
        {
            // yay, we can get native!
            return typedProperty->getNative(this);
        }
        else
        {
            // fall back to string get
            return PropertyHelper<T>::fromString(baseProperty->get(this));
        }
    }

    /*!
	\brief
		Sets the current value of a Property.

	\param name
		String containing the name of the Property who's value is to be set.

	\param value
		String containing a textual representation of the new value for the Property

	\exception UnknownObjectException	Thrown if no Property named \a name is in the PropertySet.
	\exception InvalidRequestException	Thrown when the Property was unable to interpret the content of \a value.
	*/
    void setProperty(const String& name, const String& value);

    /*!
    \copydoc PropertySet::setProperty
    
    This method tries to do a native type set without string conversion if possible,
    if that is not possible, it gracefully falls back to string conversion
    */
    template<typename T>
    void    setProperty(const String& name, typename PropertyHelper<T>::pass_type value)
    {
        PropertyRegistry::iterator pos = d_properties.find(name);

        if (pos == d_properties.end())
        {
            CEGUI_THROW(UnknownObjectException("There is no Property named '" + name + "' available in the set."));
        }

        Property* baseProperty = pos->second;
        TypedProperty<T>* typedProperty = dynamic_cast<TypedProperty<T>* >(baseProperty);

        if (typedProperty)
        {
            // yay, we can set native!
            typedProperty->setNative(this, value);
        }
        else
        {
            // fall back to string set
            baseProperty->set(this, PropertyHelper<T>::toString(value));
        }
    }

    /*!
	\brief
		Returns whether a Property is at it's default value.

	\param name
		String containing the name of the Property who's default state is to be tested.

	\return
		- true if the property has it's default value.
		- false if the property has been modified from it's default value.
	*/
    bool isPropertyDefault(const String& name) const;


    /*!
	\brief
		Returns the default value of a Property as a String.

	\param name
		String containing the name of the Property who's default string is to be returned.

	\return
		String object containing a textual representation of the default value for this property.
	*/
    String getPropertyDefault(const String& name) const;

private:
    typedef std::map<String, Property*, StringFastLessCompare
        CEGUI_MAP_ALLOC(String, Property*)> PropertyRegistry;
    PropertyRegistry	d_properties;


public:
    /*************************************************************************
		Iterator stuff
	*************************************************************************/
    typedef	ConstMapIterator<PropertyRegistry> PropertyIterator;

    /*!
    \brief
        Return a PropertySet::PropertyIterator object to iterate over the available
        Properties.
    */
    PropertyIterator getPropertyIterator(void) const;
};

/*!
\brief
    The prefered way to define properties inside class that inherit CEGUI::Window

Example of usage inside addStandardProperties or a similar method:
\code{.cpp}
{
    const String propertyOrigin("MyAwesomeClass"); // this is automatically used by the macro
    // you can also reference WidgetTypeName or any other string if applicable

    CEGUI_DEFINE_PROPERTY(Window, float, "Alpha",
        "Property to get/set the alpha value of the Window. Value is floating point number.",
        &Window::setAlpha, &Window::getAlpha, 1.0f)
};
\endcode
*/
#define CEGUI_DEFINE_PROPERTY(class_type, property_native_type, name, help, setter, getter, default_value)\
{\
    static ::CEGUI::TplWindowProperty<class_type, property_native_type> sProperty(\
            name, help, propertyOrigin, setter, getter, default_value);\
    \
    this->addProperty(&sProperty);\
}

/*!
\brief
    Similar to CEGUI_DEFINE_PROPERTY but writeXML is set to false

\note
    Properties that have no setter (null pointer is passed as setter) are automatically
    banned from XML, there is no need to use this macro for them. Doing so will cause
    CEGUI to output a warning.

Example of usage inside addStandardProperties or similar method:
\code{.cpp}
{
    const String propertyOrigin("MyAwesomeClass"); // this is automatically used by the macro
    // you can also reference WidgetTypeName or any other string if applicable

    CEGUI_DEFINE_PROPERTY_NO_XML(Window, float, "Alpha",
        "Property to get/set the alpha value of the Window. Value is floating point number.",
        &Window::setAlpha, &Window::getAlpha, 1.0f)
};
\endcode
*/
#define CEGUI_DEFINE_PROPERTY_NO_XML(class_type, property_native_type, name, help, setter, getter, default_value)\
{\
    static ::CEGUI::TplWindowProperty<class_type, property_native_type> sProperty(\
            name, help, propertyOrigin, setter, getter, default_value, false);\
    \
    this->addProperty(&sProperty);\
}

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif	// end of guard _CEGUIPropertySet_h_
