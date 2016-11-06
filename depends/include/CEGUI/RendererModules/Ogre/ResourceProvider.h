/************************************************************************
    created:    8/7/2004
    author:     James '_mental_' O'Sullivan

    purpose: Interface for Ogre specific ResourceProvider
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
#ifndef _CEGUIOgreResourceProvider_h_
#define _CEGUIOgreResourceProvider_h_

#include "../../ResourceProvider.h"
#include "CEGUI/RendererModules/Ogre/Renderer.h"

// Start of CEGUI namespace section
namespace CEGUI
{
class OGRE_GUIRENDERER_API OgreResourceProvider : public ResourceProvider
{
public:
    OgreResourceProvider();

    void loadRawDataContainer(const String& filename, RawDataContainer& output,
                              const String& resourceGroup);
    void unloadRawDataContainer(RawDataContainer& output);
    size_t getResourceGroupFileNames(std::vector<String>& out_vec,
                                     const String& file_pattern,
                                     const String& resource_group);
};

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUIOgreResourceProvider_h_
