/***********************************************************************
    created:    Sun Nov 21 2010
    author:     Paul D Turner <paul@cegui.org.uk>
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
#ifndef _CEGUILinkedEvent_h_
#define _CEGUILinkedEvent_h_

#include "CEGUI/Event.h"
#include <vector>

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{

/*!
\brief
    LinkedEvent is an Event subclass that provides a mechanism to link or chain
    Events together, such that when a specified target event is fired, then
    this event will additionally fire in response.
    \par
    The expected use of this mechanism is for container or compound widgets to
    be able to expose events of component widgets without needing to provide
    repetetive boilerplate event forwarding code, and also for scenarios where
    code is not typically provided (i.e. data based skinning scenarios).
*/
class CEGUIEXPORT LinkedEvent : public Event
{
public:
    /*!
    \brief
        Constructor

    \param name
        String object describing the name that this Event will use.

    \param target_event_set
        EventSet that the LinkedEvent should add itself to.
    */
    LinkedEvent(const String& event_name, EventSet* target_event_set);
    ~LinkedEvent();

    /*!
    \brief
        Add a link target for this event.  A link target is an event that
        will trigger this event.

    \param link_target
        Reference to an Event that, when fired, will additionally cause
        this Event to fire.

    \note
        Once an event link is established it can not currently be broken
        without destroying this Event completely.
    */
    void addLinkedTarget(Event& link_target);

protected:
    bool handler(const EventArgs& args);

    LinkedEvent(const LinkedEvent& e) : Event(e) {}

    typedef std::vector<Event::Connection
        CEGUI_VECTOR_ALLOC(Event::Connection)> LinkedConnections;
    //! collection of connection to the linked Events.
    LinkedConnections d_connections;
    //! reference to the event set that we added ourself to
    const EventSet* d_owner;
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif  // end of guard _CEGUILinkedEvent_h_

