/***********************************************************************
    created:    Tue Feb 17 2009
    author:     Paul D Turner
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2013 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUIOgreRenderer_h_
#define _CEGUIOgreRenderer_h_

#include "../../Renderer.h"
#include "../../Size.h"
#include "../../Vector.h"
#include "CEGUI/Config.h"

#include <vector>

#if (defined( __WIN32__ ) || defined( _WIN32 )) && !defined(CEGUI_STATIC)
#   ifdef CEGUIOGRERENDERER_EXPORTS
#       define OGRE_GUIRENDERER_API __declspec(dllexport)
#   else
#       define OGRE_GUIRENDERER_API __declspec(dllimport)
#   endif
#else
#   define OGRE_GUIRENDERER_API
#endif

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

namespace Ogre
{
class Root;
class RenderSystem;
class RenderTarget;
#if (CEGUI_OGRE_VERSION < ((1 << 16) | (9 << 8) | 0))
class TexturePtr;
#else
template<typename T> class SharedPtr;
class Texture;
typedef SharedPtr<Texture> TexturePtr;
#endif
class Matrix4;
}

#if (CEGUI_OGRE_VERSION >= (2 << 16))
// The new Ogre Compositor2 system has to be used since ViewPorts 
// no longer have the required functionality
#define CEGUI_USE_OGRE_COMPOSITOR2
#endif

#if (CEGUI_OGRE_VERSION >= ((2 << 16) | (1 << 8) | 0))
// The HLMS has to be used since fixed pipeline is disabled
#define CEGUI_USE_OGRE_HLMS
#include <OgreRenderOperation.h>
#include <OgreHlmsSamplerblock.h>
#endif

// Start of CEGUI namespace section
namespace CEGUI
{
class OgreGeometryBuffer;
class OgreTexture;
class OgreResourceProvider;
class OgreImageCodec;
class OgreWindowTarget;
struct OgreRenderer_impl;

//! CEGUI::Renderer implementation for the Ogre engine.
class OGRE_GUIRENDERER_API OgreRenderer : public Renderer
{
public:
#if !defined(CEGUI_USE_OGRE_COMPOSITOR2)
    /*!
    \brief
        Convenience function that creates all the Ogre specific objects and
        then initialises the CEGUI system with them.

        The created Renderer will use the default Ogre rendering window as the
        default output surface.

        This will create and initialise the following objects for you:
        - CEGUI::OgreRenderer
        - CEGUI::OgreResourceProvider
        - CEGUI::OgreImageCodec
        - CEGUI::System

    \param abi
        This must be set to CEGUI_VERSION_ABI

    \return
        Reference to the CEGUI::OgreRenderer object that was created.

    \note
        For this to succeed you must have initialised Ogre to auto create the
        rendering window.  If you have not done this, then you'll be wanting to
        use the overload that takes an Ogre::RenderTarget as input.
    */
    static OgreRenderer& bootstrapSystem(const int abi = CEGUI_VERSION_ABI);
#endif
    /*!
    \brief
        Convenience function that creates all the Ogre specific objects and
        then initialises the CEGUI system with them.

        The create Renderer will use the specified Ogre::RenderTarget as the
        default output surface.

        This will create and initialise the following objects for you:
        - CEGUI::OgreRenderer
        - CEGUI::OgreResourceProvider
        - CEGUI::OgreImageCodec
        - CEGUI::System

    \param target
        Reference to the Ogre::RenderTarget object that the created OgreRenderer
        will use as the default rendering root.

    \param abi
        This must be set to CEGUI_VERSION_ABI

    \return
        Reference to the CEGUI::OgreRenderer object that was created.
    */
    static OgreRenderer& bootstrapSystem(Ogre::RenderTarget& target,
                                         const int abi = CEGUI_VERSION_ABI);

    /*!
    \brief
        Convenience function to cleanup the CEGUI system and related objects
        that were created by calling the bootstrapSystem function.

        This function will destroy the following objects for you:
        - CEGUI::System
        - CEGUI::OgreImageCodec
        - CEGUI::OgreResourceProvider
        - CEGUI::OgreRenderer

    \note
        If you did not initialise CEGUI by calling the bootstrapSystem function,
        you should \e not call this, but rather delete any objects you created
        manually.
    */
    static void destroySystem();

#if !defined(CEGUI_USE_OGRE_COMPOSITOR2)
    /*!
    \brief
        Create an OgreRenderer object that uses the default Ogre rendering
        window as the default output surface.

    \note
        For this to succeed you must have initialised Ogre to auto create the
        rendering window.  If you have not done this, then you'll be wanting to
        use the overload that takes an Ogre::RenderTarget as input.
    */
    static OgreRenderer& create(const int abi = CEGUI_VERSION_ABI);
#endif

    /*!
    \brief
        Create an OgreRenderer object that uses the specified Ogre::RenderTarget
        as the default output surface.
    */
    static OgreRenderer& create(Ogre::RenderTarget& target,
                                const int abi = CEGUI_VERSION_ABI);

    //! destory an OgreRenderer object.
    static void destroy(OgreRenderer& renderer);

    //! function to create a CEGUI::OgreResourceProvider object
    static OgreResourceProvider& createOgreResourceProvider();

    //! function to destroy a CEGUI::OgreResourceProvider object
    static void destroyOgreResourceProvider(OgreResourceProvider& rp);

    //! function to create a CEGUI::OgreImageCodec object.
    static OgreImageCodec& createOgreImageCodec();

    //! function to destroy a CEGUI::OgreImageCodec object.
    static void destroyOgreImageCodec(OgreImageCodec& ic);

    //! set whether CEGUI rendering will occur
    void setRenderingEnabled(const bool enabled);

    //! return whether CEGUI rendering is enabled.
    bool isRenderingEnabled() const;

    /*!
    \brief
        Create a CEGUI::Texture that wraps an existing Ogre texture.

    \param name
        The name for tne new texture being created.

    \param tex
        Ogre::TexturePtr for the texture that will be used by the created
        CEGUI::Texture.

    \param take_ownership
        - true if the created Texture will assume ownership of \a tex and
        thus destroy \a tex when the Texture is destroyed.
        - false if ownership of \a tex remains with the client app, and so
        no attempt will be made to destroy \a tex when the Texture is destroyed.
    */
    Texture& createTexture(const String& name, Ogre::TexturePtr& tex,
                           bool take_ownership = false);

    //! set the render states for the specified BlendMode.
    void setupRenderingBlendMode(const BlendMode mode,
                                 const bool force = false);

    /*!
    \brief
        Controls whether rendering done by CEGUI will be wrapped with calls to
        Ogre::RenderSystem::_beginFrame and Ogre::RenderSystem::_endFrame.

        This defaults to enabled and is required when using the default hook
        that automatically calls CEGUI::System::renderGUI via a frame listener.
        If you disable this setting, the automated rendering will also be
        disabled, which is useful when you wish to perform your own calls to the
        CEGUI::System::renderGUI function (and is the sole purpose for this
        setting).

    \param enabled
        - true if _beginFrame and _endFrame should be called.
        - false if _beginFrame and _endFrame should not be called (also disables
          default renderGUI call).
    */
    void setFrameControlExecutionEnabled(const bool enabled);

    /*!
    \brief
        Returns whether rendering done by CEGUI will be wrapped with calls to
        Ogre::RenderSystem::_beginFrame and Ogre::RenderSystem::_endFrame.

        This defaults to enabled and is required when using the default hook
        that automatically calls CEGUI::System::renderGUI via a frame listener.
        If you disable this setting, the automated rendering will also be
        disabled, which is useful when you wish to perform your own calls to the
        CEGUI::System::renderGUI function (and is the sole purpose for this
        setting).

    \return
        - true if _beginFrame and _endFrame will be called.
        - false if _beginFrame and _endFrame will not be called (also means
          default renderGUI call will not be made).
    */
    bool isFrameControlExecutionEnabled() const;

    /*!
    \brief
        Sets all the required render states needed for CEGUI rendering.

        This is a low-level function intended for certain advanced concepts; in
        general it will not be required to call this function directly, since it
        is called automatically by the system when rendering is done.
    */
    void initialiseRenderStateSettings();

    /*!
    \brief
        Sets the Ogre::RenderTarget that should be targetted by the default
        GUIContext.

    \param target
        Reference to the Ogre::RenderTarget object that is to be used as the
        target for output from the default GUIContext.
    */
    void setDefaultRootRenderTarget(Ogre::RenderTarget& target);

    /*!
    \brief
        Returns whether the OgreRenderer is currently set to use shaders when
        doing its rendering operations.

    \return
        - true if rendering is being done using shaders.
        - false if rendering is being done using the fixed function pipeline
    */
    bool isUsingShaders() const;

    /*!
    \brief
        Set whether the OgreRenderer shound use shaders when performing its
        rendering operations.

    \param use_shaders
        - true if rendering shaders should be used to perform rendering.
        - false if the fixed function pipeline should be used to perform
          rendering.

    \note
        When compiled against Ogre 1.8 or later, shaders will automatically
        be enabled if the render sytem does not support the fixed function
        pipeline (such as with Direct3D 11). If you are compiling against
        earlier releases of Ogre, you must explicity enable the use of
        shaders by calling this function - if you are unsure what you'll be
        compiling against, it is safe to call this function anyway.
    */
    void setUsingShaders(const bool use_shaders);

    /*!
    \brief
        Perform required operations to bind shaders (or unbind them) depending
        on whether shader based rendering is currently enabled.

        Normally you would not need to call this function directly, although
        that might be required if you are using RenderEffect objects that
        also use shaders.
    */
    void bindShaders();

    /*!
    \brief
        Updates the shader constant parameters (i.e. uniforms).

        You do not normally need to call this function directly. Some may need
        to call this function if you're doing non-standard or advanced things.
    */
    void updateShaderParams() const;

    //! Set the current world matrix to the given matrix.
    void setWorldMatrix(const Ogre::Matrix4& m);
    //! Set the current view matrix to the given matrix.
    void setViewMatrix(const Ogre::Matrix4& m);
    //! Set the current projection matrix to the given matrix.
    void setProjectionMatrix(const Ogre::Matrix4& m);
    //! return a const reference to the current world matrix.
    const Ogre::Matrix4& getWorldMatrix() const;
    //! return a const reference to the current view matrix.
    const Ogre::Matrix4& getViewMatrix() const;
    //! return a const reference to the current projection matrix.
    const Ogre::Matrix4& getProjectionMatrix() const;

    /*!
    \brief
        Return a const reference to the final transformation matrix that
        should be used when transforming geometry.

    \note
        The projection used when building this matrix is correctly adjusted
        according to whether the current Ogre::RenderTarget requires textures
        to be flipped (i.e it does the right thing for both D3D and OpenGL).
    */
    const Ogre::Matrix4& getWorldViewProjMatrix() const;
    
    /*!
    \brief
        Returns if the texture coordinate system is vertically flipped or not. The original of a
        texture coordinate system is typically located either at the the top-left or the bottom-left.
        CEGUI, Direct3D and most rendering engines assume it to be on the top-left. OpenGL assumes it to
        be at the bottom left.        
 
        This function is intended to be used when generating geometry for rendering the TextureTarget
        onto another surface. It is also intended to be used when trying to use a custom texture (RTT)
        inside CEGUI using the Image class, in order to determine the Image coordinates correctly.

    \return
        - true if flipping is required: the texture coordinate origin is at the bottom left
        - false if flipping is not required: the texture coordinate origin is at the top left
    */
    bool isTexCoordSystemFlipped() const { return false; }

#ifdef CEGUI_USE_OGRE_HLMS
    Ogre::RenderTarget* getOgreRenderTarget();
    const Ogre::HlmsSamplerblock* getHlmsSamplerblock();
#endif

    // implement CEGUI::Renderer interface
    RenderTarget& getDefaultRenderTarget();
    GeometryBuffer& createGeometryBuffer();
    void destroyGeometryBuffer(const GeometryBuffer& buffer);
    void destroyAllGeometryBuffers();
    TextureTarget* createTextureTarget();
    void destroyTextureTarget(TextureTarget* target);
    void destroyAllTextureTargets();
    Texture& createTexture(const String& name);
    Texture& createTexture(const String& name,
                           const String& filename,
                           const String& resourceGroup);
    Texture& createTexture(const String& name, const Sizef& size);
    void destroyTexture(Texture& texture);
    void destroyTexture(const String& name);
    void destroyAllTextures();
    Texture& getTexture(const String& name) const;
    bool isTextureDefined(const String& name) const;
    void beginRendering();
    void endRendering();
    void setDisplaySize(const Sizef& sz);
    const Sizef& getDisplaySize() const;
    const Vector2f& getDisplayDPI() const;
    uint getMaxTextureSize() const;
    const String& getIdentifierString() const;

protected:
    //! default constructor.
    OgreRenderer();
    //! constructor takin the Ogre::RenderTarget to use as the default root.
    OgreRenderer(Ogre::RenderTarget& target);
    //! destructor.
    virtual ~OgreRenderer();

    //! checks Ogre initialisation.  throws exceptions if an issue is detected.
    void checkOgreInitialised();
    //! helper to throw exception if name is already used.
    void throwIfNameExists(const String& name) const;
    //! helper to safely log the creation of a named texture
    static void logTextureCreation(const String& name);
    //! helper to safely log the destruction of a named texture
    static void logTextureDestruction(const String& name);

    //! common parts of constructor
    void constructor_impl(Ogre::RenderTarget& target);
    //! helper that creates and sets up shaders
    void initialiseShaders();
    //! helper to clean up shaders
    void cleanupShaders();

    //! Pointer to the hidden implementation data
    OgreRenderer_impl* d_pimpl;
};


} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif  // end of guard _CEGUIOgreRenderer_h_
