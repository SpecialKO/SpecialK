/***********************************************************************
    created:    21/2/2004
    author:     Paul D Turner

    purpose:    Defines class for a named collection of Event objects
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
#ifndef _CEGUIEventSet_h_
#define _CEGUIEventSet_h_

#include "CEGUI/Base.h"
#include "CEGUI/String.h"
#include "CEGUI/Event.h"
#include "CEGUI/IteratorBase.h"
#include <map>

#if defined (_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251 4521 4522)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    Interface providing event signaling and handling

    The EventSet is a means for code to attach a handler function to some
    named event, and later, for that event to be fired and the subscribed
    handler(s) called.

    Its purpose is similar to Qt's signal and slot system, you can think of
    Event name as the signal and the subscribed handler as the slot.

    Each Event has a name and a set of handlers. Handlers can be free functions,
    class member functions or even functors. Whenever an Event is fired all of
    its handlers are invoked. You are most likely looking for a way to react
    to mouse clicks or other events fired internally by CEGUI.

    The handlers have to have a very specific signature. They have to return
    bool and they have to take const EventArgs&. If this is not met you will
    encounter build errors!

\par Reacting to internal CEGUI events

\code{.cpp}
bool handler(const EventArgs& args)
{
    std::cout << "Don't you dare click me again!" << std::endl;

    // returning true means we handled the Event and it doesn't need to be
    // propagated any further
    return true;
}

void example()
{
    CEGUI::Window* wnd = ...;
    wnd->subscribeEvent(CEGUI::Window::EventMouseClick, &handler);
}
\endcode

\par A contrived example of various handler types

\code{.cpp}
bool freeFunction(const EventArgs& args)
{
    // your handler code here
}

// nothing special about this class!
class CustomClass
{
    public:
        bool memberFunction(const EventArgs& args)
        {
            // your handler code here
        }

        static bool staticMemberFunction(const EventArgs& args)
        {
            // your handler code here
        }

        bool memberFunctionWithArg(const EventArgs& args, bool something)
        {
            // your handler code here
        }
};

class Functor
{
    public:
        bool operator()(const EventArgs& args)
        {
            // your handler code here
        }
}

void example()
{
    CustomClass instance;

    CEGUI::Window* wnd = ...;

    // creates "CustomEvent", subscribes freeFunction to it
    wnd->subscribeEvent("CustomEvent", &freeFunction);
    // subscribes memberFunction of "instance" to CustomEvent
    wnd->subscribeEvent("CustomEvent", &CustomClass::memberFunction, &instance);
    // subscribes staticMemberFunction of CustomClass to CustomEvent
    wnd->subscribeEvent("CustomEvent", &CustomClass::staticMemberFunction);
    // subscribes Functor to CustomEvent, Functor instance is owned by EventSet
    // after this call, it will be destroyed by EventSet
    wnd->subscribeEvent("CustomEvent", Functor());

    // Advanced subscribers using boost::bind follow:

    // subscribes memberFunctionWithArg of CustomClass to CustomEvent
    wnd->subscribeEvent("CustomEvent", boost::bind(&CustomClass::memberFunctionWithArg, _1, _2, true), &instance);
    // same as above, binding the instance itself as well
    wnd->subscribeEvent("CustomEvent", boost::bind(&CustomClass::memberFunctionWithArg, instance, _1, true));

    // the following line causes all subscribed handlers to be called
    wnd->fireEvent("CustomEvent");
}
\endcode

\par EventSet works well with others
    The EventSet can also subscribe boost::function, std::function, functors
    created by boost::bind and possibly others. This is not a feature of
    CEGUI per se, just a fortunate side effect of being able to call functors.

\par Events are automatically created on demand
    As of CEGUI 0.5, the EventSet no longer needs to be filled with available events.
    Events are now added to the set as they are first used; that is, the first
    time a handler is subscribed to an event for a given EventSet, an Event
    object is created and added to the EventSet.

    Instead of throwing an exception when firing an event that does not actually
    exist in the set, we now do nothing (if the Event does not exist, then it
    has no handlers subscribed, and therefore doing nothing is the correct
    course action).
*/
class CEGUIEXPORT EventSet
{
public:
    /*!
    \brief
        Constructor for EventSet objects
    */
    EventSet();

    /*!
    \brief
        Destructor for EventSet objects
    */
    virtual ~EventSet(void);

    /*!
    \brief
        Creates a new Event object with the given name and adds it to the
        EventSet.

    \param name
        String object containing the name to give the new Event.  The name must
        be unique for the EventSet.

    \exception AlreadyExistsException
        Thrown if an Event already exists named \a name.
    */
    void addEvent(const String& name);

    /*!
    \brief
        Adds the given Event object to the EventSet.  Ownership of the object
        passes to EventSet and it will be deleted when it is removed from the
        EventSet - whether explicitly via removeEvent or when the EventSet
        is destroyed.

    \param event
        Reference to an Event or Event based object that is to be added to the
        EventSaet

    \exception AlreadyExistsException
        Thrown if the EventSet already contains an Event with the same name
        as \a event.  Note that \a event will be destroyed under this scenario.
    */
    void addEvent(Event& event);

    /*!
    \brief
        Removes the Event with the given name.  All connections to the event
        are disconnected, and the underlying Event object is destroyed.

    \param name
        String object containing the name of the Event to remove.  If no such
        Event exists, nothing happens.
    */
    void removeEvent(const String& name);

    /*!
    \brief
        Removes the given event from the EventSet.  All connections to the event
        are disconnected, and the event object is destroyed.

    \param event
        Reference to the Event or Event based object to be removed from the
        EventSet.
    */
    void removeEvent(Event& event);

    /*!
    \brief
        Remove all Event objects from the EventSet.  Add connections will be
        disconnected, and all Event objects destroyed.
    */
    void removeAllEvents(void);

    /*!
    \brief
        Checks to see if an Event with the given name is present in this
        EventSet.

    \return
        - true if an Event named \a name is defined for this EventSet.
        - false if no Event named \a name is defined for this EventSet.
    */
    bool isEventPresent(const String& name);

    /*!
    \brief
        Subscribes a handler to the named Event.  If the named Event is not yet
        present in the EventSet, it is created and added.

    \param name
        String object containing the name of the Event to subscribe to.

    \param subscriber
        Function or object that is to be subscribed to the Event.

    \return
        Connection object that can be used to check the status of the Event
        connection and to disconnect (unsubscribe) from the Event.
    */
    virtual Event::Connection subscribeEvent(const String& name,
                                             Event::Subscriber subscriber);

    /*!
    \brief
        Subscribes a handler to the specified group of the named Event.  If the
        named Event is not yet present in the EventSet, it is created and added.

    \param name
        String object containing the name of the Event to subscribe to.

    \param group
        Group which is to be subscribed to.  Subscription groups are called in
        ascending order.

    \param subscriber
        Function or object that is to be subscribed to the Event.

    \return
        Connection object that can be used to check the status of the Event
        connection and to disconnect (unsubscribe) from the Event.
    */
    virtual Event::Connection subscribeEvent(const String& name,
                                             Event::Group group,
                                             Event::Subscriber subscriber);

    /*!
    \copydoc EventSet::subscribeEvent
    
    \internal This is there just to make the syntax a tad easier
    */
    template<typename Arg1, typename Arg2>
    inline Event::Connection subscribeEvent(const String& name, Arg1 arg1, Arg2 arg2)
    {
        return subscribeEvent(name, Event::Subscriber(arg1, arg2));
    }

    /*!
    \copydoc EventSet::subscribeEvent
    
    \internal This is there just to make the syntax a tad easier
    */
    template<typename Arg1, typename Arg2>
    inline Event::Connection subscribeEvent(const String& name, Event::Group group, Arg1 arg1, Arg2 arg2)
    {
        return subscribeEvent(name, group, Event::Subscriber(arg1, arg2));
    }

    /*!
    \brief
        Subscribes the named Event to a scripted funtion

    \param name
        String object containing the name of the Event to subscribe to.

    \param subscriber_name
        String object containing the name of the script funtion that is to be
        subscribed to the Event.

    \return
        Connection object that can be used to check the status of the Event
        connection and to disconnect (unsubscribe) from the Event.
    */
    virtual Event::Connection subscribeScriptedEvent(const String& name,
                                                     const String& subscriber_name);

    /*!
    \brief
        Subscribes the specified group of the named Event to a scripted funtion.

    \param name
        String object containing the name of the Event to subscribe to.

    \param group
        Group which is to be subscribed to.  Subscription groups are called in
        ascending order.

    \param subscriber_name
        String object containing the name of the script funtion that is to be
        subscribed to the Event.

    \return
        Connection object that can be used to check the status of the Event
        connection and to disconnect (unsubscribe) from the Event.
    */
    virtual Event::Connection subscribeScriptedEvent(const String& name,
                                                     Event::Group group,
                                                     const String& subscriber_name);

    /*!
    \brief
        Fires the named event passing the given EventArgs object.

    \param name
        String object holding the name of the Event that is to be fired
        (triggered)

    \param args
        The EventArgs (or derived) object that is to be bassed to each
        subscriber of the Event.  Once all subscribers
        have been called the 'handled' field of the event is updated
        appropriately.

    \param eventNamespace
        String object describing the global event namespace prefix for this
        event.
    */
    virtual void fireEvent(const String& name, EventArgs& args,
                           const String& eventNamespace = "");


    /*!
    \brief
        Return whether the EventSet is muted or not.

    \return
        - true if the EventSet is muted.  All requests to fire events will be
          ignored.
        - false if the EventSet is not muted.  Requests to fire events are
          processed as normal.
    */
    bool isMuted(void) const;

    /*!
    \brief
        Set the mute state for this EventSet.

    \param setting
        - true if the EventSet is to be muted (no further event firing requests
          will be honoured until EventSet is unmuted).
        - false if the EventSet is not to be muted and all events should fired
          as requested.
    */
    void    setMutedState(bool setting);

    /*!
    \brief
        Return a pointer to the Event object with the given name, optionally
        adding such an Event object to the EventSet if it does not already
        exist.

    \param name
        String object holding the name of the Event to return.

    \param autoAdd
        - true if an Event object named \a name should be added to the set
          if such an Event does not currently exist.
        - false if no object should automatically be added to the set.  In this
          case, if the Event does not already exist 0 will be returned.

    \return
        Pointer to the Event object in this EventSet with the specifed name.
        Or 0 if such an Event does not exist and \a autoAdd was false.
    */
    Event* getEventObject(const String& name, bool autoAdd = false);

protected:
    //! Implementation event firing member
    void fireEvent_impl(const String& name, EventArgs& args);
    //! Helper to return the script module pointer or throw.
    ScriptModule* getScriptModule() const;

    // Do not allow copying, assignment, or any other usage than simple creation.
    EventSet(EventSet&) {} //! \deprecated
    EventSet(const EventSet&) {}
    EventSet& operator=(EventSet&) { return *this; } //! \deprecated
    EventSet& operator=(const EventSet&) { return *this; }

    typedef std::map<String, Event*, StringFastLessCompare
        CEGUI_MAP_ALLOC(String, Event*)> EventMap;
    EventMap    d_events;

    bool d_muted;    //!< true if events for this EventSet have been muted.

public:
    /*************************************************************************
        Iterator stuff
    *************************************************************************/
    typedef ConstMapIterator<EventMap> EventIterator;

    /*!
    \brief
        Return a EventSet::EventIterator object to iterate over the events currently
        added to the EventSet.
    */
    EventIterator getEventIterator(void) const;
};

} // End of  CEGUI namespace section


#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif  // end of guard _CEGUIEventSet_h_
