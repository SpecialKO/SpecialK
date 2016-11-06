/***********************************************************************
    created:    Tue Feb 17 2009
    author:     Paul D Turner
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
#ifndef _CEGUIOgreWindowTarget_h_
#define _CEGUIOgreWindowTarget_h_

#include "CEGUI/RendererModules/Ogre/RenderTarget.h"

// Start of CEGUI namespace section
namespace CEGUI
{
//! CEGUI::RenderTarget that targets an existing gre::RenderTarget
class OGRE_GUIRENDERER_API OgreWindowTarget : public OgreRenderTarget<>
{
public:
    //! Constructor
    OgreWindowTarget(OgreRenderer& owner, Ogre::RenderSystem& rs,
                     Ogre::RenderTarget& target);

    //! Destructor
    virtual ~OgreWindowTarget();

    /*!
    \brief
        Set the Ogre::RenderTarget that the output from the OgreWindowTarget
        should be rendered to.

    \param target
        Reference to an Ogre::RenderTarget object that will receive the rendered
        output.
    */
    void setOgreRenderTarget(Ogre::RenderTarget& target);

    // implement parts of CEGUI::RenderTarget interface
    bool isImageryCache() const;

protected:
    //! helper function to initialise the render target details
    void initRenderTarget(Ogre::RenderTarget& target);
};

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUIOgreWindowTarget_h_
