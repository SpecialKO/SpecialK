/***********************************************************************
    created:    Fri Mar 02 2012
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
#ifndef _CEGUIEventAction_h_
#define _CEGUIEventAction_h_

#include "CEGUI/String.h"
#include "CEGUI/Event.h"
#include "CEGUI/falagard/Enums.h"

#include <map>

#if defined (_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

namespace CEGUI
{
class Window;

class CEGUIEXPORT EventAction :
    public AllocatedObject<EventAction>
{
public:
    EventAction(const String& event_name, ChildEventAction action);
    ~EventAction();

    void setEventName(const String& event_name);
    const String& getEventName() const;

    void setAction(ChildEventAction action);
    ChildEventAction getAction() const;

    void initialiseWidget(Window& widget) const;
    void cleanupWidget(Window& widget) const;

    void writeXMLToStream(XMLSerializer& xml_stream) const;

protected:
    String d_eventName;
    ChildEventAction d_action;

    String makeConnectionKeyName(const Window& widget) const;

    typedef std::multimap<String, Event::ScopedConnection> ConnectionMap;
    mutable ConnectionMap d_connections;
};

}

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif

