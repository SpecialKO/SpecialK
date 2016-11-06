/***********************************************************************
    created:    Sun Jan 11 2009
    authors:    Paul D Turner <paul@cegui.org.uk>
                Lukas E Meindl
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
#ifndef _CEGUIOpenGLRenderer_h_
#define _CEGUIOpenGLRenderer_h_

#include "RendererBase.h"

namespace CEGUI
{
/*!
\brief
    Renderer class to interface with desktop OpenGL
*/
class OPENGL_GUIRENDERER_API OpenGLRenderer : public OpenGLRendererBase
{
public:
    //! Enumeration of valid texture target types.
    enum TextureTargetType
    {
        //! Automatically choose the best type available.
        TTT_AUTO,
        //! Use targets based on frame buffer objects if available, else none.
        TTT_FBO,
        //! Use targets based on pbuffer support if available, else none.
        TTT_PBUFFER,
        //! Disable texture targets.
        TTT_NONE
    };

    /*!
    \brief
        Convenience function that creates the required objects to initialise the
        CEGUI system.

        The created Renderer will use the current OpenGL viewport as it's
        default surface size.

        This will create and initialise the following objects for you:
        - CEGUI::OpenGLRenderer
        - CEGUI::DefaultResourceProvider
        - CEGUI::System

    \param tt_type
        Specifies one of the TextureTargetType enumerated values indicating the
        desired TextureTarget type to be used.  Defaults to TTT_AUTO.

    \param abi
        This must be set to CEGUI_VERSION_ABI

    \return
        Reference to the CEGUI::OpenGLRenderer object that was created.
    */
    static OpenGLRenderer& bootstrapSystem(
                                    const TextureTargetType tt_type = TTT_AUTO,
                                    const int abi = CEGUI_VERSION_ABI);

    /*!
    \brief
        Convenience function that creates the required objects to initialise the
        CEGUI system.

        The created Renderer will use the current OpenGL viewport as it's
        default surface size.

        This will create and initialise the following objects for you:
        - CEGUI::OpenGLRenderer
        - CEGUI::DefaultResourceProvider
        - CEGUI::System

    \param display_size
        Size object describing the initial display resolution.

    \param tt_type
        Specifies one of the TextureTargetType enumerated values indicating the
        desired TextureTarget type to be used.  Defaults to TTT_AUTO.

    \param abi
        This must be set to CEGUI_VERSION_ABI

    \return
        Reference to the CEGUI::OpenGLRenderer object that was created.
    */
    static OpenGLRenderer& bootstrapSystem(const Sizef& display_size,
                                           const TextureTargetType tt_type = TTT_AUTO,
                                           const int abi = CEGUI_VERSION_ABI);

    /*!
    \brief
        Convenience function to cleanup the CEGUI system and related objects
        that were created by calling the bootstrapSystem function.

        This function will destroy the following objects for you:
        - CEGUI::System
        - CEGUI::DefaultResourceProvider
        - CEGUI::OpenGLRenderer

    \note
        If you did not initialise CEGUI by calling the bootstrapSystem function,
        you should \e not call this, but rather delete any objects you created
        manually.
    */
    static void destroySystem();

    /*!
    \brief
        Create an OpenGLRenderer object.

    \param tt_type
        Specifies one of the TextureTargetType enumerated values indicating the
        desired TextureTarget type to be used.

    \param abi
        This must be set to CEGUI_VERSION_ABI
    */
    static OpenGLRenderer& create(const TextureTargetType tt_type = TTT_AUTO,
                                  const int abi = CEGUI_VERSION_ABI);

    /*!
    \brief
        Create an OpenGLRenderer object.

    \param display_size
        Size object describing the initial display resolution.

    \param tt_type
        Specifies one of the TextureTargetType enumerated values indicating the
        desired TextureTarget type to be used.

    \param abi
        This must be set to CEGUI_VERSION_ABI
    */
    static OpenGLRenderer& create(const Sizef& display_size,
                                  const TextureTargetType tt_type = TTT_AUTO,
                                  const int abi = CEGUI_VERSION_ABI);

    /*!
    \brief
        Destroy an OpenGLRenderer object.

    \param renderer
        The OpenGLRenderer object to be destroyed.
    */
    static void destroy(OpenGLRenderer& renderer);

    // base class overrides / abstract function implementations
    void beginRendering();
    void endRendering();
    bool isS3TCSupported() const;
    void setupRenderingBlendMode(const BlendMode mode,
                                 const bool force = false);
    void setViewProjectionMatrix(const mat4Pimpl* viewProjectionMatrix);

protected:
    //! Overrides
    OpenGLGeometryBufferBase* createGeometryBuffer_impl();
    TextureTarget* createTextureTarget_impl();

    void initialiseRendererIDString();

    /*!
    \brief
        Constructor for OpenGL Renderer objects

    \param tt_type
        Specifies one of the TextureTargetType enumerated values indicating the
        desired TextureTarget type to be used.
    */
    OpenGLRenderer(const TextureTargetType tt_type);

    /*!
    \brief
        Constructor for OpenGL Renderer objects.

    \param display_size
        Size object describing the initial display resolution.

    \param tt_type
        Specifies one of the TextureTargetType enumerated values indicating the
        desired TextureTarget type to be used.
    */
    OpenGLRenderer(const Sizef& display_size, const TextureTargetType tt_type);

    /*!
    \brief
        Destructor for OpenGLRenderer objects
    */
    virtual ~OpenGLRenderer();

    //! init the extra GL states enabled via enableExtraStateSettings
    void setupExtraStates();

    //! cleanup the extra GL states enabled via enableExtraStateSettings
    void cleanupExtraStates();

    //! initialise OGLTextureTargetFactory that will generate TextureTargets
    void initialiseTextureTargetFactory(const TextureTargetType tt_type);

    void initialiseGLExtensions();

    //! pointer to a helper that creates TextureTargets supported by the system.
    OGLTextureTargetFactory* d_textureTargetFactory;
};

}

#endif

