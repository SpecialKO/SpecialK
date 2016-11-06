/***********************************************************************
    created:    19/3/2010
    author:     Jeff A. Marr

    purpose:    Defines a resource provider for minizip archives
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
#ifndef _CEGUIMinizipResourceProvider_h_
#define _CEGUIMinizipResourceProvider_h_

#include "CEGUI/Base.h"
#include "CEGUI/DefaultResourceProvider.h"

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

// NOTE: While the DefaultResourceProvider (DRP) was not originally intended
// to be derived from, the purpose of this class is to extend the behavior of
// the default implementation. The only difference is that this class will
// attempt to load resources from a zip archive if the resource does not exist
// outside the archive.

// Start of CEGUI namespace section
namespace CEGUI
{
class CEGUIEXPORT MinizipResourceProvider : public DefaultResourceProvider
{
public:
    /*************************************************************************
        Construction and Destruction
    *************************************************************************/
    MinizipResourceProvider();
    MinizipResourceProvider(const String& archive, bool loadLocal = true);
    ~MinizipResourceProvider(void);

    /*!
    \brief
        sets the archive from which files are retrieved.

    \param archive
        The filepath to the archive
    */
    void setArchive(const String& archive);
    void setLoadLocal(bool load = true);

    void loadRawDataContainer(const String& filename,
                              RawDataContainer& output,
                              const String& resourceGroup);
    size_t getResourceGroupFileNames(std::vector<String>& out_vec,
                                     const String& file_pattern,
                                     const String& resource_group);
protected:
    bool doesFileExist(const String& filename);
    void openArchive();
    void closeArchive();

    struct Impl;
    Impl* d_pimpl;
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif  // end of guard _CEGUIMinizipResourceProvider_h_

