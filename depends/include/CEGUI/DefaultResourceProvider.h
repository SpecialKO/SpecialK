/***********************************************************************
	created:	8/7/2004
	author:		James '_mental_' O'Sullivan
	
	purpose:	Defines abstract base class for CEGUIData objects
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
#ifndef _CEGUIDefaultResourceProvider_h_
#define _CEGUIDefaultResourceProvider_h_

#include "CEGUI/Base.h"
#include "CEGUI/ResourceProvider.h"

#include <map>

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
class CEGUIEXPORT DefaultResourceProvider : public ResourceProvider
{
public:
	/*************************************************************************
		Construction and Destruction
	*************************************************************************/
	DefaultResourceProvider() {}
	~DefaultResourceProvider(void) {}

    /*!
    \brief
        Set the directory associated with a given resource group identifier.

    \param resourceGroup
        The resource group identifier whose directory is to be set.

    \param directory
        The directory to be associated with resource group identifier
        \a resourceGroup

    \return
        Nothing.
    */
    void setResourceGroupDirectory(const String& resourceGroup, const String& directory);

    /*!
    \brief
        Return the directory associated with the specified resource group
        identifier.

    \param resourceGroup
        The resource group identifier for which the associated directory is to
        be returned.

    \return
        String object describing the directory currently associated with resource
        group identifier \a resourceGroup.

    \note
        This member is not defined as being const because it may cause
        creation of an 'empty' directory specification for the resourceGroup
        if the resourceGroup has not previously been accessed.
    */
    const String& getResourceGroupDirectory(const String& resourceGroup);

    /*!
    \brief
        clears any currently set directory for the specified resource group
        identifier.

    \param resourceGroup
        The resource group identifier for which the associated directory is to
        be cleared.
    */
    void clearResourceGroupDirectory(const String& resourceGroup);

    void loadRawDataContainer(const String& filename, RawDataContainer& output, const String& resourceGroup);
    void unloadRawDataContainer(RawDataContainer& data);
    size_t getResourceGroupFileNames(std::vector<String>& out_vec,
                                     const String& file_pattern,
                                     const String& resource_group);

protected:
    /*!
    \brief
        Return the final path and filename, taking into account the given
        resource group identifier that should be used when attempting to
        load the data.
    */
    String getFinalFilename(const String& filename, const String& resourceGroup) const;

    typedef std::map<String, String, StringFastLessCompare> ResourceGroupMap;
    ResourceGroupMap    d_resourceGroups;
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#endif	// end of guard _CEGUIDefaultResourceProvider_h_
