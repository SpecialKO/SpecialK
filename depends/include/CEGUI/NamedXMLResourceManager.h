/***********************************************************************
    created:    Fri Jul 17 2009
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
#ifndef _CEGUINamedXMLResourceManager_h_
#define _CEGUINamedXMLResourceManager_h_

#include "CEGUI/EventSet.h"
#include "CEGUI/String.h"
#include "CEGUI/Exceptions.h"
#include "CEGUI/Logger.h"
#include "CEGUI/InputEvent.h"
#include "CEGUI/System.h"
#include <map>

// Start of CEGUI namespace section
namespace CEGUI
{
//! Possible actions when loading an XML resource that already exists.
enum XMLResourceExistsAction
{
    //! Do not load the resource, return the existing instance.
    XREA_RETURN,
    //! Destroy the existing instance and replace with the newly loaded one.
    XREA_REPLACE,
    //! Throw an AlreadyExistsException.
    XREA_THROW
};

//----------------------------------------------------------------------------//

//! implementation class to gather EventSet parts for all template instances.
class CEGUIEXPORT ResourceEventSet : public EventSet
{
public:
    //! Namespace name for all resource managers
    static const String EventNamespace;
    /** Name of event fired when a resource is created by this manager.
     * Handlers are passed a const ResourceEventArgs reference with
     * ResourceEventArgs::resourceType String set to the type of resource that
     * the event is related to, and ResourceEventArgs::resourceName String set
     * to the name of the resource that the event is related to.
     */
    static const String EventResourceCreated;
    /** Name of event fired when a resource is destroyed by this manager.
     * Handlers are passed a const ResourceEventArgs reference with
     * ResourceEventArgs::resourceType String set to the type of resource that
     * the event is related to, and ResourceEventArgs::resourceName String set
     * to the name of the resource that the event is related to.
     */
    static const String EventResourceDestroyed;
    /** Name of event fired when a resource is replaced by this manager.
     * Handlers are passed a const ResourceEventArgs reference with
     * ResourceEventArgs::resourceType String set to the type of resource that
     * the event is related to, and ResourceEventArgs::resourceName String set
     * to the name of the resource that the event is related to.
     */
    static const String EventResourceReplaced;
};

//----------------------------------------------------------------------------//

/*!
\brief
    Templatised manager class that loads and manages named XML based resources.

\tparam T
    Type for the object that is loaded and managed by the class.

\tparam U
    Type for the loader used to create object instances of T from XML files.
    This class does all of the creation / loading work and can be of any form
    or type desired, with the following requirements:
    - a constructor signature compatible with the following call:
      U(const String& xml_filename, const String& resource_group);
    - a function getObjectName that returns the name of the object as specified
      in the XML file given in the constructor to U.
    - a function getObject that returns a reference to the object created as a
      result of processing the XML file given in the constructor to U.
\note
    Once NamedXMLResourceManager calls getObject on the instance of U, it
    assumes that it now owns the object returned, if no call to getObject is
    made, no such transfer of ownership is assumed.
*/
template<typename T, typename U>
class NamedXMLResourceManager : public ResourceEventSet
{
public:
    /*!
    \brief
        Constructor.

    \param resource_type
        String object holding some textual indication of the type of objects
        managed by the collection.  This is used to give more descriptive
        log and exception messages.
    */
    NamedXMLResourceManager(const String& resource_type);

    //! Destructor.
    virtual ~NamedXMLResourceManager();

    /*!
    \brief
        Creates a new T object from a RawDataContainer and adds it to the collection.

        Use an instance of the xml resource loading class \a U to process the
        XML source thereby creating an instance of class \a T and add it to the collection under
        the name specified in the XML file.

    \param source
        RawDataContainer holding the XML source to be used when creating the
        new object instance.

    \param action
        One of the XMLResourceExistsAction enumerated values indicating what
        action should be taken when an object with the specified name
        already exists within the collection.
    */
    T& createFromContainer(const RawDataContainer& source,
                           XMLResourceExistsAction action = XREA_RETURN);

    /*!
    \brief
        Creates a new T object from an XML file and adds it to the collection.

        Use an instance of the xml resource loading class \a U to process the
        XML file \a xml_filename from resource group \a resource_group thereby
        creating an instance of class \a T and add it to the collection under
        the name specified in the XML file.

    \param xml_filename
        String holding the filename of the XML file to be used when creating the
        new object instance.

    \param resource_group
        String holding the name of the resource group identifier to be used
        when loading the XML file described by \a xml_filename.

    \param action
        One of the XMLResourceExistsAction enumerated values indicating what
        action should be taken when an object with the specified name
        already exists within the collection.
    */
    T& createFromFile(const String& xml_filename, const String& resource_group = "",
                      XMLResourceExistsAction action = XREA_RETURN);

    /*!
    \brief
        Creates a new T object from a string and adds it to the collection.

        Use an instance of the xml resource loading class \a U to process the
        XML source thereby creating an instance of class \a T and add it to the collection under
        the name specified in the XML file.

    \param source
        String holding the XML source to be used when creating the
        new object instance.

    \param action
        One of the XMLResourceExistsAction enumerated values indicating what
        action should be taken when an object with the specified name
        already exists within the collection.
    */
    T& createFromString(const String& source,
                        XMLResourceExistsAction action = XREA_RETURN);

    /*!
    \brief
        Destroy the object named \a object_name, or do nothing if such an
        object does not exist in the collection.

    \param object_name
        String holding the name of the object to be destroyed.
    */
    void destroy(const String& object_name);

    /*!
    \brief
        Destroy the object \a object, or do nothing if such an
        object does not exist in the collection.

    \param object
        The object to be destroyed (beware of keeping references to this object
        once it's been destroyed!)
    */
    void destroy(const T& object);

    //! Destroy all objects.
    void destroyAll();

    /*!
    \brief
        Return a reference to the object named \a object_name.

    \param object_name
        String holding the name of the object to be returned.

    \exception UnknownObjectException
        thrown if no object named \a object_name exists within the collection.
    */
    T& get(const String& object_name) const;

    //! Return whether an object named \a object_name exists.
    bool isDefined(const String& object_name) const;

    //! Create a new T object from files with names matching \a pattern in \a resource_group
    void createAll(const String& pattern, const String& resource_group);

protected:
    // singleton allocator fits here, resource managers are very likely to be singletons
    //! type of collection used to store and manage objects
    typedef std::map<String, T*, StringFastLessCompare
        CEGUI_MAP_ALLOC(String, T*)> ObjectRegistry;
    //! implementation of object destruction.
    void destroyObject(typename ObjectRegistry::iterator ob);
    //! function to enforce XMLResourceExistsAction policy.
    T& doExistingObjectAction(const String object_name, T* object,
                              const XMLResourceExistsAction action);
    //! Function called each time a new object is added to the collection.
    virtual void doPostObjectAdditionAction(T& object);
    //! String holding the text for the resource type managed.
    const String d_resourceType;
    //! the collection of objects
    ObjectRegistry d_objects;
};

//----------------------------------------------------------------------------//
template<typename T, typename U>
NamedXMLResourceManager<T, U>::NamedXMLResourceManager(
        const String& resource_type) :
    d_resourceType(resource_type)
{
}

//----------------------------------------------------------------------------//
template<typename T, typename U>
NamedXMLResourceManager<T, U>::~NamedXMLResourceManager()
{
}

//----------------------------------------------------------------------------//
template<typename T, typename U>
T& NamedXMLResourceManager<T, U>::createFromContainer(const RawDataContainer& source,
                                                      XMLResourceExistsAction action)
{
    U xml_loader;

    xml_loader.handleContainer(source);
    return doExistingObjectAction(xml_loader.getObjectName(),
                                  &xml_loader.getObject(), action);
}

//----------------------------------------------------------------------------//
template<typename T, typename U>
T& NamedXMLResourceManager<T, U>::createFromFile(const String& xml_filename,
                                                 const String& resource_group,
                                                 XMLResourceExistsAction action)
{
    U xml_loader;

    xml_loader.handleFile(xml_filename, resource_group);
    return doExistingObjectAction(xml_loader.getObjectName(),
                                  &xml_loader.getObject(), action);
}

//----------------------------------------------------------------------------//
template<typename T, typename U>
T& NamedXMLResourceManager<T, U>::createFromString(const String& source,
                                                   XMLResourceExistsAction action)
{
    U xml_loader;

    xml_loader.handleString(source);
    return doExistingObjectAction(xml_loader.getObjectName(),
                                  &xml_loader.getObject(), action);
}

//----------------------------------------------------------------------------//
template<typename T, typename U>
void NamedXMLResourceManager<T, U>::destroy(const String& object_name)
{
    typename ObjectRegistry::iterator i(d_objects.find(object_name));

    // exit if no such object.
    if (i == d_objects.end())
        return;

    destroyObject(i);
}

//----------------------------------------------------------------------------//
template<typename T, typename U>
void NamedXMLResourceManager<T, U>::destroy(const T& object)
{
    // don't want to force a 'getName' function on T here, so we'll look for the
    // object the hard way.
    typename ObjectRegistry::iterator i(d_objects.begin());
    for (; i != d_objects.end(); ++i)
        if (i->second == &object)
        {
            destroyObject(i);
            return;
        }
}

//----------------------------------------------------------------------------//
template<typename T, typename U>
void NamedXMLResourceManager<T, U>::destroyAll()
{
    while (!d_objects.empty())
        destroyObject(d_objects.begin());
}

//----------------------------------------------------------------------------//
template<typename T, typename U>
T& NamedXMLResourceManager<T, U>::get(const String& object_name) const
{
    typename ObjectRegistry::const_iterator i(d_objects.find(object_name));

    if (i == d_objects.end())
        CEGUI_THROW(UnknownObjectException(
            "No object of type '" + d_resourceType + "' named '" + object_name +
            "' is present in the collection."));

    return *i->second;
}

//----------------------------------------------------------------------------//
template<typename T, typename U>
bool NamedXMLResourceManager<T, U>::isDefined(const String& object_name) const
{
    return d_objects.find(object_name) != d_objects.end();
}

//----------------------------------------------------------------------------//
template<typename T, typename U>
void NamedXMLResourceManager<T, U>::destroyObject(
        typename ObjectRegistry::iterator ob)
{
    char addr_buff[32];
    sprintf(addr_buff, "(%p)", static_cast<void*>(ob->second));
    Logger::getSingleton().logEvent("Object of type '" + d_resourceType +
        "' named '" + ob->first + "' has been destroyed. " +
        addr_buff, Informative);

    // Set up event args for event notification
    ResourceEventArgs args(d_resourceType, ob->first);

    CEGUI_DELETE_AO ob->second;
    d_objects.erase(ob);

    // fire event signalling an object has been destroyed
    fireEvent(EventResourceDestroyed, args, EventNamespace);
}

//----------------------------------------------------------------------------//
template<typename T, typename U>
T& NamedXMLResourceManager<T, U>::doExistingObjectAction(
    const String object_name,
    T* object,
    const XMLResourceExistsAction action)
{
    String event_name;

    if (isDefined(object_name))
    {
        switch (action)
        {
        case XREA_RETURN:
            Logger::getSingleton().logEvent("---- Returning existing instance "
                "of " + d_resourceType + " named '" + object_name + "'.");
            // delete any new object we already had created
            CEGUI_DELETE_AO object;
            // return existing instance of object.
            return *d_objects[object_name];

        case XREA_REPLACE:
            Logger::getSingleton().logEvent("---- Replacing existing instance "
                "of " + d_resourceType + " named '" + object_name +
                "' (DANGER!).");
            destroy(object_name);
            event_name = EventResourceReplaced;
            break;

        case XREA_THROW:
            CEGUI_DELETE_AO object;
            CEGUI_THROW(AlreadyExistsException(
                "an object of type '" + d_resourceType + "' named '" +
                object_name + "' already exists in the collection."));

        default:
            CEGUI_DELETE_AO object;
            CEGUI_THROW(InvalidRequestException(
                "Invalid CEGUI::XMLResourceExistsAction was specified."));
        }
    }
    else
        event_name = EventResourceCreated;

    d_objects[object_name] = object;
    doPostObjectAdditionAction(*object);

    // fire event about this resource change
    ResourceEventArgs args(d_resourceType, object_name);
    fireEvent(event_name, args, EventNamespace);

    return *object;
}

//----------------------------------------------------------------------------//
template<typename T, typename U>
void NamedXMLResourceManager<T, U>::doPostObjectAdditionAction(T& /*object*/)
{
    // do nothing by default.
}

//----------------------------------------------------------------------------//
template<typename T, typename U>
void NamedXMLResourceManager<T, U>::createAll(const String& pattern,
                                              const String& resource_group)
{
    std::vector<String> names;
    const size_t num = System::getSingleton().getResourceProvider()->
        getResourceGroupFileNames(names, pattern, resource_group);

    for (size_t i = 0; i < num; ++i)
        createFromFile(names[i], resource_group);
}

//----------------------------------------------------------------------------//

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUINamedXMLResourceManager_h_
