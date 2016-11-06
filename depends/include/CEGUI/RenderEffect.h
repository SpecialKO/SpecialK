/***********************************************************************
    created:    Sat Jan 17 2009
    author:     Paul D Turner
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
#ifndef _CEGUIRenderEffect_h_
#define _CEGUIRenderEffect_h_

#include "CEGUI/Base.h"

// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    Interface for objects that hook into RenderingWindow to affect the rendering
    process, thus allowing various effects to be achieved.
*/
class CEGUIEXPORT RenderEffect :
    public AllocatedObject<RenderEffect>
{
public:
    virtual ~RenderEffect() {};
    /*!
    \brief
        Return the number of passes required by this effect.

    \return
        integer value indicating the number of rendering passes required to
        fully render this effect.
    */
    virtual int getPassCount() const = 0;

    /*!
    \brief
        Function called prior to RenderingWindow::draw being called.  This is
        intended to be used for any required setup / state initialisation and is
        called once for each pass in the effect.

    \param pass
        Indicates the pass number to be initialised (starting at pass 0).

    \note
        Note that this function is called \e after any standard state
        initialisation that might be peformed by the Renderer module.
    */
    virtual void performPreRenderFunctions(const int pass) = 0;

    /*!
    \brief
        Function called after RenderingWindow::draw is called.  This is intended
        to be used for any required cleanup / state restoration.  This function
        is called <em>once only</em>, unlike performPreRenderFunctions which may
        be called multiple times; once for each pass in the effect.
    \note
        Note that this function is called \e before any standard state
        cleanup that might be peformed by the Renderer module.
    */
    virtual void performPostRenderFunctions() = 0;

    /*!
    \brief
        Function called to generate geometry for the RenderingWindow.

        The geometry generated should be fully unclipped and window local.  The
        origin for the geometry is located at the top-left corner.

    \param window
        The RenderingWindow object that is being processed.

    \param geometry
        GeometryBuffer object where the generated geometry should be added.
        This object will be cleared before this function is invoked.

    \return
        boolean value indicating whether the RenderingWindow should generate
        it's own geometry.
        - true if the RenderingWindow should generate it's own geometry.  You
        will usually only return true if you do not need to use custom geometry.
        - false if you have added any required geometry needed to represent the
        RenderingWindow.
    */
    virtual bool realiseGeometry(RenderingWindow& window,
                                 GeometryBuffer& geometry) = 0;

    /*!
    \brief
        Function called to perform any time based updates on the RenderEffect
        state.

    \note
        This function should only affect the internal state of the RenderEffect
        object.  This function should definitely \e not be used to directly
        affect any render states of the underlying rendering API or engine.

    \param elapsed
        The number of seconds that have elapsed since the last time this
        function was called.

    \param window
        RenderingWindow object that the RenderEffect is being applied to.

    \return
        boolean that indicates whether the window geometry will still be valid
        after the update.
    */
    virtual bool update(const float elapsed, RenderingWindow& window) = 0;
};

} // End of  CEGUI namespace section

#endif  // end of guard _CEGUIRenderEffect_h_
