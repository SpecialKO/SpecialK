/***********************************************************************
	created:	16/1/2005
	author:		Paul D Turner
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
#ifndef _CEGUIGlobalEventSet_h_
#define _CEGUIGlobalEventSet_h_

#include "CEGUI/EventSet.h"
#include "CEGUI/Singleton.h"


#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4275)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    The GlobalEventSet singleton allows you to subscribe to an event for all
    instances of a class.  The GlobalEventSet effectively supports "late binding"
    to events; which means you can subscribe to some event that does not actually
    exist (yet).
*/
class CEGUIEXPORT GlobalEventSet :
    public EventSet,
    public Singleton<GlobalEventSet>,
    public AllocatedObject<GlobalEventSet>
{
public:
	GlobalEventSet();
	~GlobalEventSet();


	/*!
	\brief
		Return singleton System object

	\return
		Singleton System object
	*/
	static	GlobalEventSet&	getSingleton(void);


	/*!
	\brief
		Return pointer to singleton System object

	\return
		Pointer to singleton System object
	*/
	static	GlobalEventSet*	getSingletonPtr(void);


	/*!
	\brief
		Fires the named event passing the given EventArgs object.

	\param name
		String object holding the name of the Event that is to be fired (triggered)

	\param args
		The EventArgs (or derived) object that is to be bassed to each subscriber of the Event.  Once all subscribers
		have been called the 'handled' field of the event is updated appropriately.
		
	\param eventNamespace
		String object describing the namespace prefix to use when firing the global event.

	\return
		Nothing.
	*/
	virtual void	fireEvent(const String& name, EventArgs& args, const String& eventNamespace = "");
};

} // End of  CEGUI namespace section


#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif	// end of guard _CEGUIGlobalEventSet_h_
