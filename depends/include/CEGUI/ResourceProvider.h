/***********************************************************************
	created:	8/7/2004
	author:		James '_mental_' O'Sullivan
	
	purpose:	Defines abstract base class for loading DataContainer objects
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2011 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUIResourceProvider_h_
#define _CEGUIResourceProvider_h_

#include "CEGUI/Base.h"
#include "CEGUI/DataContainer.h"
#include "CEGUI/String.h"
#include <vector>


// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
	Abstract class that defines the required interface for all resource provider sub-classes.

	A ResourceProvider is used to load both XML and binary data from an external source.  This could be from a filesystem or the resource manager of a specific renderer.
*/
class CEGUIEXPORT ResourceProvider :
    public AllocatedObject<ResourceProvider>
{
public:
	/*************************************************************************
		Construction and Destruction
	*************************************************************************/
    /*!
    \brief
        Constructor for the ResourceProvider class
    */
	ResourceProvider() { }

    /*!
    \brief
        Destructor for the ResourceProvider class
    */
	virtual ~ResourceProvider(void) { }

    /*************************************************************************
        Accessor functions
    *************************************************************************/

//    /*!
//    \brief
//        Load XML data using InputSource objects.
//
//    \param filename
//        String containing a filename of the resource to be loaded.
//
//	\param output
//		Reference to a InputSourceContainer object to load the data into.
//   */
//    virtual void loadInputSourceContainer(const String& filename, InputSourceContainer& output) = 0;

    /*!
    \brief
        Load raw binary data.

    \param filename
        String containing a filename of the resource to be loaded.

	\param output
        Reference to a RawDataContainer object to load the data into.

    \param resourceGroup
        Optional String that may be used by implementations to identify the group from
        which the resource should be loaded.
    */
    virtual void loadRawDataContainer(const String& filename, RawDataContainer& output, const String& resourceGroup) = 0;

    /*!
    \brief
        Unload raw binary data. This gives the resource provider a change to unload the data
        in its own way before the data container object is destroyed.  If it does nothing,
        then the object will release its memory.

	\param data
        Reference to a RawDataContainer object that is about to be destroyed.

    */
    virtual void unloadRawDataContainer(RawDataContainer&)  { }

    /*!
    \brief
        Return the current default resource group identifier.

    \return
        String object containing the currently set default resource group identifier.
    */
    const String&   getDefaultResourceGroup(void) const     { return d_defaultResourceGroup; }
    
    /*!
    \brief
        Set the default resource group identifier.

    \param resourceGroup
        String object containing the default resource group identifier to be used.

    \return
        Nothing.
    */
    void    setDefaultResourceGroup(const String& resourceGroup)    { d_defaultResourceGroup = resourceGroup; }

    /** enumerate the files in \a resource_group that match \a file_pattern and
    append thier names to \a out_vec
    */
    virtual size_t getResourceGroupFileNames(std::vector<String>& out_vec,
                                             const String& file_pattern,
                                             const String& resource_group) = 0;
protected:
    String  d_defaultResourceGroup;     //!< Default resource group identifier.
};

} // End of  CEGUI namespace section

#endif	// end of guard _CEGUIResourceProvider_h_
