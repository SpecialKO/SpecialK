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
#ifndef _CEGUIOpenGLESRenderer_h_
#define _CEGUIOpenGLESRenderer_h_

#include "CEGUI/Base.h"
#include "CEGUI/Renderer.h"
#include "CEGUI/Size.h"
#include "CEGUI/Vector.h"
#include "CEGUI/RendererModules/OpenGLES/GLES.h"
#include <vector>
#include <map>

#if (defined( __WIN32__ ) || defined( _WIN32 )) && !defined(CEGUI_STATIC)
#   ifdef CEGUIOPENGLESRENDERER_EXPORTS
#       define OPENGLES_GUIRENDERER_API __declspec(dllexport)
#   else
#       define OPENGLES_GUIRENDERER_API __declspec(dllimport)
#   endif
#else
#   define OPENGLES_GUIRENDERER_API
#endif

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif


// Start of CEGUI namespace section
namespace CEGUI
{
class OpenGLESTexture;
class OpenGLESTextureTarget;
class OpenGLESGeometryBuffer;
class OGLTextureTargetFactory;

/*!
\brief
    Renderer class to interface with OpenGLES
*/
class OPENGLES_GUIRENDERER_API OpenGLESRenderer : public Renderer
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

        The created Renderer will use the current OpenGL ES viewport as it's
        default surface size.

        This will create and initialise the following objects for you:
        - CEGUI::OpenGLESRenderer
        - CEGUI::DefaultResourceProvider
        - CEGUI::System

    \param tt_type
        Specifies one of the TextureTargetType enumerated values indicating the
        desired TextureTarget type to be used.  Defaults to TTT_AUTO.

    \param abi
        This must be set to CEGUI_VERSION_ABI

    \return
        Reference to the CEGUI::OpenGLESRenderer object that was created.
    */
    static OpenGLESRenderer& bootstrapSystem(
                                const TextureTargetType tt_type = TTT_AUTO,
                                const int abi = CEGUI_VERSION_ABI);

    /*!
    \brief
        Convenience function that creates the required objects to initialise the
        CEGUI system.

        The created Renderer will use /a display_size as the default surface
        size.

        This will create and initialise the following objects for you:
        - CEGUI::OpenGLESRenderer
        - CEGUI::DefaultResourceProvider
        - CEGUI::System

    \param display_size
        Size object describing the initial display dimensions.

    \param tt_type
        Specifies one of the TextureTargetType enumerated values indicating the
        desired TextureTarget type to be used.  Defaults to TTT_AUTO.

    \param abi
        This must be set to CEGUI_VERSION_ABI

    \return
        Reference to the CEGUI::OpenGLESRenderer object that was created.
    */
    static OpenGLESRenderer& bootstrapSystem(
                                const Sizef& display_size,
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
        Create an OpenGLESRenderer object.

    \param tt_type
        Specifies one of the TextureTargetType enumerated values indicating the
        desired TextureTarget type to be used.

    \param abi
        This must be set to CEGUI_VERSION_ABI
    */
    static OpenGLESRenderer& create(const TextureTargetType tt_type = TTT_AUTO,
                                    const int abi = CEGUI_VERSION_ABI);

    /*!
    \brief
        Create an OpenGLESRenderer object.

    \param display_size
        Size object describing the initial display resolution.

    \param tt_type
        Specifies one of the TextureTargetType enumerated values indicating the
        desired TextureTarget type to be used.

    \param abi
        This must be set to CEGUI_VERSION_ABI
    */
    static OpenGLESRenderer& create(const Sizef& display_size,
                                    const TextureTargetType tt_type = TTT_AUTO,
                                    const int abi = CEGUI_VERSION_ABI);

    /*!
    \brief
        Destroy an OpenGLESRenderer object.

    \param renderer
        The OpenGLESRenderer object to be destroyed.
    */
    static void destroy(OpenGLESRenderer& renderer);

	/*!
    \brief
        Check if provided extension is supported on current platform.
		Khronos reference implementation.

    \param extension
        Provided extension string identification
    */
	static bool isGLExtensionSupported( const char* extension );

    // implement Renderer interface
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

    /*!
    \brief
        Create a texture that uses an existing OpenGLES texture with the specified
        size.  Note that it is your responsibility to ensure that the OpenGLES
        texture is valid and that the specified size is accurate.

    \param sz
        Size object that describes the pixel size of the OpenGLES texture
        identified by \a tex.

    \return
        Texture object that wraps the OpenGLES texture \a tex, and whose size is
        specified to be \a sz.
    */
    Texture& createTexture(const String& name, GLuint tex, const Sizef& sz);

    /*!
    \brief
        Tells the renderer to initialise some extra states beyond what it
        directly needs itself for CEGUI.

        This option is useful in cases where you've made changes to the default
        OpenGLES state and do not want to save/restore those between CEGUI
        rendering calls.  Note that this option will not deal with every
        possible state or extension - if you want a state added here, make a
        request and we'll consider it ;)
    */
    void enableExtraStateSettings(bool setting);

    /*!
    \brief
        Grabs all the loaded textures from Texture RAM and stores them in a
        local data buffer.  This function invalidates all textures, and
        restoreTextures must be called before any CEGUI rendering is done for
        predictable results.
    */
    void grabTextures();

    /*!
    \brief
        Restores all the loaded textures from the local data buffers previously
        created by 'grabTextures'
    */
    void restoreTextures();

    /*!
    \brief
        Helper to return a valid texture size according to reported OpenGLES
        capabilities.

    \param sz
        Size object containing input size.

    \return
        Size object containing - possibly different - output size.
    */
    Sizef getAdjustedTextureSize(const Sizef& sz) const;

    /*!
    \brief
        Utility function that will return \a f if it's a power of two, or the
        next power of two up from \a f if it's not.
    */
    static float getNextPOTSize(const float f);
    
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
    bool isTexCoordSystemFlipped() const { return true; }

private:
    /*!
    \brief
        Constructor for OpenGLES Renderer objects

    \param tt_type
        Specifies one of the TextureTargetType enumerated values indicating the
        desired TextureTarget type to be used.
    */
    OpenGLESRenderer(const TextureTargetType tt_type);

    /*!
    \brief
        Constructor for OpenGLES Renderer objects.

    \param display_size
        Size object describing the initial display resolution.

    \param tt_type
        Specifies one of the TextureTargetType enumerated values indicating the
        desired TextureTarget type to be used.
    */
    OpenGLESRenderer(const Sizef& display_size, const TextureTargetType tt_type);

    /*!
    \brief
        Destructor for OpenGLESRenderer objects
    */
    virtual ~OpenGLESRenderer();

    //! init the extra GL states enabled via enableExtraStateSettings
    void setupExtraStates();

    //! cleanup the extra GL states enabled via enableExtraStateSettings
    void cleanupExtraStates();

    //! initialise OGLTextureTargetFactory that will generate TextureTargets
    void initialiseTextureTargetFactory(const TextureTargetType tt_type);

    //! Log about the fact we destroyed a named texture.
    void logTextureDestruction(const String& name);

	//! Replacement for glPushAttrib =(
	struct RenderStates
	{
		GLboolean glScissorTest;
		GLboolean texturing;
		GLboolean blend;
		GLint arrayBuffer;
		GLint texture;
		GLint texEnvParam;
	} glPreRenderStates;

    //! String holding the renderer identification text.
    static String d_rendererID;
    //! What the renderer considers to be the current display size.
    Sizef d_displaySize;
    //! What the renderer considers to be the current display DPI resolution.
    Vector2f d_displayDPI;
    //! The default RenderTarget
    RenderTarget* d_defaultTarget;
    //! container type used to hold TextureTargets we create.
    typedef std::vector<TextureTarget*> TextureTargetList;
    //! Container used to track texture targets.
    TextureTargetList d_textureTargets;
    //! container type used to hold GeometryBuffers we create.
    typedef std::vector<OpenGLESGeometryBuffer*> GeometryBufferList;
    //! Container used to track geometry buffers.
    GeometryBufferList d_geometryBuffers;
    //! container type used to hold Textures we create.
    typedef std::map<String, OpenGLESTexture*, StringFastLessCompare
                     CEGUI_MAP_ALLOC(String, OpenGLESTexture*)> TextureMap;
    //! Container used to track textures.
    TextureMap d_textures;
    //! What the renderer thinks the max texture size is.
    uint d_maxTextureSize;
    //! option of whether to initialise extra states that may not be at default
    bool d_initExtraStates;
    //! pointer to a helper that creates TextureTargets supported by the system.
    OGLTextureTargetFactory* d_textureTargetFactory;
  };

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif // end of guard _CEGUIOpenGLESRenderer_h_

