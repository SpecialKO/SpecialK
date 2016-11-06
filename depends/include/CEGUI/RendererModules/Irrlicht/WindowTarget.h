/***********************************************************************
    created:    Tue Mar 3 2009
    author:     Paul D Turner (parts based on original code by Thomas Suter)
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
#ifndef _CEGUIIrrlichtWindowTarget_h_
#define _CEGUIIrrlichtWindowTarget_h_

#include "CEGUI/RendererModules/Irrlicht/RendererDef.h"
#include "CEGUI/RendererModules/Irrlicht/RenderTarget.h"

// Start of CEGUI namespace section
namespace CEGUI
{
//! CEGUI::RenderTarget that targets a window, or a part of a window
class IRR_GUIRENDERER_API IrrlichtWindowTarget : public IrrlichtRenderTarget<>
{
public:
    //! Constructor
    IrrlichtWindowTarget(IrrlichtRenderer& owner,
                         irr::video::IVideoDriver& driver);
    //! Destructor
    virtual ~IrrlichtWindowTarget();

    // implement parts of CEGUI::RenderTarget interface
    bool isImageryCache() const;
};

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUIIrrlichtWindowTarget_h_
