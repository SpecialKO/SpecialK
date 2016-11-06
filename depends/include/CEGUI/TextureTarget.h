/***********************************************************************
    created:    Sun Jan 11 2009
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
#ifndef _CEGUITextureTarget_h_
#define _CEGUITextureTarget_h_

#include "CEGUI/RenderTarget.h"

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    Specialisation of RenderTarget interface that should be used as the base
    class for RenderTargets that are implemented using textures.
*/
class CEGUIEXPORT TextureTarget : public RenderTarget
{
public:
    /*!
    \brief
        Clear the surface of the underlying texture.
    */
    virtual void clear() = 0;

    /*!
    \brief
        Return a pointer to the CEGUI::Texture that the TextureTarget is using.

    \return
        Texture object that the TextureTarget uses when rendering imagery.
    */
    virtual Texture& getTexture() const = 0;

    /*!
    \brief
        Used to declare to the TextureTarget the largest size, in pixels, of the
        next set of incoming rendering operations.

    \note
        The main purpose of this is to allow for the implemenatation to resize
        the underlying texture so that it can hold the imagery that will be
        drawn.

    \param sz
        Size object describing the largest area that will be rendererd in the
        next batch of rendering operations.

    \exception InvalidRequestException
        May be thrown if the TextureTarget would not be able to handle the
        operations rendering content of the given size.
    */
    virtual void declareRenderSize(const Sizef& sz) = 0;

    /*!
    \brief
        Return whether rendering done on the target texture is inverted in
        relation to regular textures.

        This is intended to be used when generating geometry for rendering the
        TextureTarget onto another surface.

    \return
        - true if the texture content should be considered as inverted
        vertically in comparison with other regular textures.
        - false if the texture content has the same orientation as regular
        textures.
        
    \deprecated
        This function is deprecated.
    */
    virtual bool isRenderingInverted() const = 0;
};

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUITextureTarget_h_
