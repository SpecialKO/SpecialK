/***********************************************************************
    created:    Tue Aug 18 2009
    author:     Paul D Turner <paul@cegui.org.uk>
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
#ifndef _CEGUIIrrlichtImageCodec_h_
#define _CEGUIIrrlichtImageCodec_h_

#include "../../ImageCodec.h"
#include "CEGUI/RendererModules/Irrlicht/RendererDef.h"

// forward declararions of irrlicht bits.
namespace irr
{
namespace video
{
    class IVideoDriver;
}
}

// Start of CEGUI namespace section
namespace CEGUI
{
//! ImageCodec object that loads data via image loading facilities in Irrlicht.
class IRR_GUIRENDERER_API IrrlichtImageCodec : public ImageCodec
{
public:
    //! Constructor.
    IrrlichtImageCodec(irr::video::IVideoDriver& driver);

    // implement required function from ImageCodec.
    Texture* load(const RawDataContainer& data, Texture* result);

protected:
    irr::video::IVideoDriver& d_driver;
};

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUIIrrlichtImageCodec_h_
