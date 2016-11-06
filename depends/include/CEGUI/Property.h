/***********************************************************************
	created:	21/2/2004
	author:		Paul D Turner
	
	purpose:	Defines the Property class which forms part of a
				PropertySet
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
#ifndef _CEGUIProperty_h_
#define _CEGUIProperty_h_

#include "CEGUI/Base.h"
#include "CEGUI/String.h"
#include "CEGUI/XMLSerializer.h" 

// Start of CEGUI namespace section
namespace CEGUI
{

/*!
\brief
	Dummy base class to ensure correct casting of receivers.
*/
class CEGUIEXPORT PropertyReceiver
{
public:
	PropertyReceiver() {}
	virtual ~PropertyReceiver() {}
};


/*!
\brief
	An abstract class that defines the interface to access object properties by name.

	Property objects allow (via a PropertySet) access to certain properties of objects
	by using simple get/set functions and the name of the property to be accessed.
*/
class CEGUIEXPORT Property :
    public AllocatedObject<Property>
{
public:
    static const String XMLElementName;
    static const String NameXMLAttributeName;
    static const String ValueXMLAttributeName;

	/*!
	\brief
		Creates a new Property object.

	\param name
		String containing the name of the new Property.

	\param help
		String containing a description of the Property and it's usage.
		
	\param defaultValue
		String holding the textual representation of the default value for this Property

    \param writesXML
        Specifies whether the writeXMLToStream method should do anything for this Property.  This
        enables selectivity in what properties within a PropertySet will get output as XML.

	\param dataType
	    String representation of the data type this property is held in ("int", "UVector2", ...)

	\param origin
	    String describing the origin class of this Property (Window, FrameWindow, ...)
	*/
	Property(const String& name, const String& help, const String& defaultValue = "", bool writesXML = true, const String& dataType = "Unknown", const String& origin = "Unknown") :
	  d_name(name),
	  d_help(help),
	  d_default(defaultValue),
	  d_writeXML(writesXML),
      d_dataType(dataType),
      d_origin(origin)
    {}

	/*!
	\brief
		Destructor for Property objects
	*/
	virtual ~Property(void) {}


	/*!
	\brief
		Return a String that describes the purpose and usage of this Property.

	\return
		String that contains the help text
	*/
	const String& getHelp(void) const		{return d_help;}


	/*!
	\brief
		Return a the name of this Property

	\return
		String containing the name of the Property
	*/
	const String& getName(void) const		{return d_name;}

    /*!
	\brief
		Return string data type of this Property

	\return
		String containing the data type of the Property
	*/
    const String& getDataType(void) const   {return d_dataType;}

    /*!
    \brief
        Return string origin of this Property

    \return
        String containing the origin of the Property
    */
    const String& getOrigin(void) const   {return d_origin;}

	/*!
	\brief
		Return the current value of the Property as a String

	\param receiver
		Pointer to the target object.

	\return
		String object containing a textual representation of the current value of the Property
	*/
	virtual String	get(const PropertyReceiver* receiver) const = 0;


	/*!
	\brief
		Sets the value of the property

	\param receiver
		Pointer to the target object.

	\param value
		A String object that contains a textual representation of the new value to assign to the Property.

	\return
		Nothing.

	\exception InvalidRequestException	Thrown when the Property was unable to interpret the content of \a value.
	*/
	virtual void	set(PropertyReceiver* receiver, const String& value) = 0;


	/*!
	\brief
		Returns whether the property is at it's default value.

	\param receiver
		Pointer to the target object.

	\return
		- true if the property has it's default value.
		- false if the property has been modified from it's default value.
	*/
	virtual bool	isDefault(const PropertyReceiver* receiver) const;


	/*!
	\brief
		Returns the default value of the Property as a String.

	\param receiver
		Pointer to the target object.

	\return
		String object containing a textual representation of the default value for this property.
	*/
	virtual String	getDefault(const PropertyReceiver* receiver) const;


    /*!
    \brief
        Writes out an XML representation of this class to the given stream.

    \note
        This would normally have been implemented via XMLGenerator base class, but in this
        case we require the target PropertyReceiver in order to obtain the property value.
    */
    virtual void writeXMLToStream(const PropertyReceiver* receiver, XMLSerializer& xml_stream) const;

    /*!
    \brief
        Returns whether the property is readable.

    \return
        - true if the property is readable.
        - false if the property isn't readable.
    */
    virtual bool isReadable() const;

    /*!
    \brief
        Returns whether the property is writable.

    \return
        - true if the property is writable.
        - false if the property isn't writable.
    */
    virtual bool isWritable() const;

    /*!
    \brief
    	Returns whether the property writes to XML streams.
    */
    virtual bool doesWriteXML() const;

    //! function to allow initialisation of a PropertyReceiver.
    virtual void initialisePropertyReceiver(PropertyReceiver* /*receiver*/) const {}

    virtual Property* clone() const = 0;

protected:
	String d_name;		//!< String that stores the Property name.
	String d_help;		//!< String that stores the Property help text.
	String d_default;	//!< String that stores the Property default value string.
	bool d_writeXML; //!< Specifies whether writeXMLToStream should do anything for this property.
    // TODO: This is really ugly but PropertyDefinition forced me to do this to support operator=
    String d_dataType; //!< Holds data type of this property
    // TODO: This is really ugly but PropertyDefinition forced me to do this to support operator=
    String d_origin; //!< Holds origin of this property
};

} // End of  CEGUI namespace section

#endif	// end of guard _CEGUIProperty_h_
