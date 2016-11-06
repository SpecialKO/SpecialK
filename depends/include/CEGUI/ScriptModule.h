/***********************************************************************
	created:	16/7/2004
	author:		Paul D Turner

	purpose:	Abstract class interface for scripting support
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
#ifndef _CEGUIScriptModule_h_
#define _CEGUIScriptModule_h_

#include "CEGUI/Base.h"
#include "CEGUI/String.h"
#include "CEGUI/Event.h"


// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
	Abstract interface required for all scripting support modules to be used with
	the CEGUI system.
*/
class CEGUIEXPORT ScriptModule :
    public AllocatedObject<ScriptModule>
{
public:
	/*************************************************************************
		Construction and Destruction
	*************************************************************************/
	/*!
	\brief
		Constructor for ScriptModule base class
	*/
	ScriptModule(void);


	/*!
	\brief
		Destructor for ScriptModule base class.
	*/
	virtual ~ScriptModule(void) {}


	/*************************************************************************
		Abstract interface
	*************************************************************************/
	/*!
	\brief
		Execute a script file.

	\param filename
		String object holding the filename of the script file that is to be executed
		
	\param resourceGroup
		Resource group idendifier to be passed to the ResourceProvider when loading the script file.
	*/
	virtual	void	executeScriptFile(const String& filename, const String& resourceGroup = "")	= 0;


	/*!
	\brief
		Execute a scripted global function.  The function should not take any parameters and should return an integer.

	\param function_name
		String object holding the name of the function, in the global script environment, that
		is to be executed.

	\return
		The integer value returned from the script function.
	*/
	virtual int	executeScriptGlobal(const String& function_name)	= 0;


	/*!
	\brief
		Execute a scripted global 'event handler' function.  The function should take some kind of EventArgs like parameter
		that the concrete implementation of this function can create from the passed EventArgs based object.  The function
		should not return anything.

	\param handler_name
		String object holding the name of the scripted handler function.

	\param e
		EventArgs based object that should be passed, by any appropriate means, to the scripted function.

	\return
		- true if the event was handled.
		- false if the event was not handled.
	*/
	virtual	bool	executeScriptedEventHandler(const String& handler_name, const EventArgs& e)		= 0;


    /*!
    \brief
        Execute script code contained in the given CEGUI::String object.

    \param str
        String object holding the valid script code that should be executed.

    \return
        Nothing.
    */
    virtual void executeString(const String& str) = 0;


    /*!
    \brief
        Method called during system initialisation, prior to running any scripts via the ScriptModule, to enable the ScriptModule
        to perform any operations required to complete initialisation or binding of the script language to the gui system objects.

    \return
        Nothing.
    */
    virtual void createBindings(void) {}


    /*!
    \brief
        Method called during system destruction, after all scripts have been run via the ScriptModule, to enable the ScriptModule
        to perform any operations required to cleanup bindings of the script language to the gui system objects, as set-up in the
        earlier createBindings call.

    \return
        Nothing.
    */
    virtual void destroyBindings(void) {}

    /*!
    \brief
        Return identification string for the ScriptModule.  If the internal id string has not been
        set by the ScriptModule creator, a generic string of "Unknown scripting module" will be returned.

    \return
        String object holding a string that identifies the ScriptModule in use.
    */
    const String& getIdentifierString() const;

    /*!
    \brief
            Subscribes the named Event to a scripted funtion

    \param target
            The target EventSet for the subscription.

    \param name
            String object containing the name of the Event to subscribe to.

    \param subscriber_name
            String object containing the name of the script funtion that is to be subscribed to the Event.

    \return
            Connection object that can be used to check the status of the Event connection and to disconnect (unsubscribe) from the Event.

    \exception UnknownObjectException	Thrown if an Event named \a name is not in the EventSet
    */
    virtual Event::Connection	subscribeEvent(EventSet* target, const String& name, const String& subscriber_name) = 0;

    /*!
    \brief
            Subscribes the specified group of the named Event to a scripted funtion.

    \param target
            The target EventSet for the subscription.

    \param name
            String object containing the name of the Event to subscribe to.

    \param group
            Group which is to be subscribed to.  Subscription groups are called in ascending order.

    \param subscriber_name
            String object containing the name of the script funtion that is to be subscribed to the Event.

    \return
            Connection object that can be used to check the status of the Event connection and to disconnect (unsubscribe) from the Event.

    \exception UnknownObjectException	Thrown if an Event named \a name is not in the EventSet
    */
    virtual Event::Connection	subscribeEvent(EventSet* target, const String& name, Event::Group group, const String& subscriber_name) = 0;

    /*!
    \brief
        Sets the default resource group to be used when loading script files.

    \param resourceGroup
        String describing the default resource group identifier to be used.

    \return
        Nothing.
    */
    static void setDefaultResourceGroup(const String& resourceGroup)
        { d_defaultResourceGroup = resourceGroup; }

    /*!
    \brief
        Returns the default resource group used when loading script files.

    \return
        String describing the default resource group identifier..
    */
    static const String& getDefaultResourceGroup()
        { return d_defaultResourceGroup; }

protected:
    //! String that holds some id information about the module.
    String d_identifierString;
    //! holds the default resource group ID for loading script files.
    static String d_defaultResourceGroup;
};


/*!
\brief
	Functor class used for binding named script functions to events
*/
class CEGUIEXPORT ScriptFunctor :
    public AllocatedObject<ScriptFunctor>
{
public:
	ScriptFunctor(const String& functionName) : scriptFunctionName(functionName) {}
    ScriptFunctor(const ScriptFunctor& obj) : scriptFunctionName(obj.scriptFunctionName) {}
	bool	operator()(const EventArgs& e) const;

private:
    // no assignment possible
    ScriptFunctor& operator=(const ScriptFunctor& rhs);

	const String	scriptFunctionName;
};

} // End of  CEGUI namespace section


#endif	// end of guard _CEGUIScriptModule_h_
