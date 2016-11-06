/***********************************************************************
    created:    Sat Mar 7 2009
    author:     Paul D Turner (parts based on code by Rajko Stojadinovic)
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
#ifndef _CEGUIDirect3D10Renderer_h_
#define _CEGUIDirect3D10Renderer_h_

#include "../../Renderer.h"
#include "../../Size.h"
#include "../../Vector.h"
#include <vector>
#include <map>

#if (defined( __WIN32__ ) || defined( _WIN32 )) && !defined(CEGUI_STATIC)
#   ifdef CEGUIDIRECT3D10RENDERER_EXPORTS
#       define D3D10_GUIRENDERER_API __declspec(dllexport)
#   else
#       define D3D10_GUIRENDERER_API __declspec(dllimport)
#   endif
#else
#   define D3D10_GUIRENDERER_API
#endif

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

// D3D forward refs
struct ID3D10Device;
struct ID3D10Effect;
struct ID3D10EffectTechnique;
struct ID3D10InputLayout;
struct ID3D10EffectShaderResourceVariable;
struct ID3D10EffectMatrixVariable;
struct ID3D10ShaderResourceView;
struct D3DXMATRIX;

// Start of CEGUI namespace section
namespace CEGUI
{
class Direct3D10GeometryBuffer;
class Direct3D10Texture;

//! Renderer implementation using Direct3D 10.
class D3D10_GUIRENDERER_API Direct3D10Renderer : public Renderer
{
public:
    /*!
    \brief
        Convenience function that creates the required objects to initialise the
        CEGUI system.

        This will create and initialise the following objects for you:
        - CEGUI::Direct3D10Renderer
        - CEGUI::DefaultResourceProvider
        - CEGUI::System

    \param device
        Pointer to the ID3D10Device interface that is to be used for CEGUI
        rendering operations.

    \param abi
        This must be set to CEGUI_VERSION_ABI

    \return
        Reference to the CEGUI::Direct3D10Renderer object that was created.
    */
    static Direct3D10Renderer& bootstrapSystem(ID3D10Device* device,
                                               const int abi = CEGUI_VERSION_ABI);

    /*!
    \brief
        Convenience function to cleanup the CEGUI system and related objects
        that were created by calling the bootstrapSystem function.

        This function will destroy the following objects for you:
        - CEGUI::System
        - CEGUI::DefaultResourceProvider
        - CEGUI::Direct3D10Renderer

    \note
        If you did not initialise CEGUI by calling the bootstrapSystem function,
        you should \e not call this, but rather delete any objects you created
        manually.
    */
    static void destroySystem();

    /*!
    \brief
        Create an Direct3D10Renderer object.
    */
    static Direct3D10Renderer& create(ID3D10Device* device,
                                      const int abi = CEGUI_VERSION_ABI);

    /*!
    \brief
        Destroy an Direct3D10Renderer object.

    \param renderer
        The Direct3D10Renderer object to be destroyed.
    */
    static void destroy(Direct3D10Renderer& renderer);
    
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

    //! return the ID3D10Device used by this renderer object.
    ID3D10Device& getDirect3DDevice() const;

    //! low-level function that binds the technique pass ready for use
    void bindTechniquePass(const BlendMode mode, const bool clipped);
    //! low-level function to set the texture shader resource view to be used.
    void setCurrentTextureShaderResource(ID3D10ShaderResourceView* srv); 
    //! low-level function to set the projection matrix to be used.
    void setProjectionMatrix(D3DXMATRIX& matrix);
    //! low-level function to set the world matrix to be used.
    void setWorldMatrix(D3DXMATRIX& matrix);

    // Implement interface from Renderer
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
    //! constructor
    Direct3D10Renderer(ID3D10Device* device);

    //! destructor.
    ~Direct3D10Renderer();

    //! return size of the D3D device viewport.
    Sizef getViewportSize();

    //! helper to throw exception if name is already used.
    void throwIfNameExists(const String& name) const;
    //! helper to safely log the creation of a named texture
    static void logTextureCreation(const String& name);
    //! helper to safely log the destruction of a named texture
    static void logTextureDestruction(const String& name);

    //! String holding the renderer identification text.
    static String d_rendererID;
    //! The D3D device we're using to render with.
    ID3D10Device* d_device;
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
    typedef std::vector<Direct3D10GeometryBuffer*> GeometryBufferList;
    //! Container used to track geometry buffers.
    GeometryBufferList d_geometryBuffers;
    //! container type used to hold Textures we create.
    typedef std::map<String, Direct3D10Texture*, StringFastLessCompare
                     CEGUI_MAP_ALLOC(String, Direct3D10Texture*)> TextureMap;
    //! Container used to track textures.
    TextureMap d_textures;
    //! Effect (shader) used when rendering.
    ID3D10Effect* d_effect;
    //! Rendering technique that supplies scissor clipped BM_NORMAL type rendering
    ID3D10EffectTechnique* d_normalClippedTechnique;
    //! Rendering technique that supplies BM_NORMAL type rendering
    ID3D10EffectTechnique* d_normalUnclippedTechnique;
    //! Rendering technique that supplies scissor clipped BM_RTT_PREMULTIPLIED type rendering
    ID3D10EffectTechnique* d_premultipliedClippedTechnique;
    //! Rendering technique that supplies BM_RTT_PREMULTIPLIED type rendering
    ID3D10EffectTechnique* d_premultipliedUnclippedTechnique;
    //! D3D10 input layout describing the vertex format we use.
    ID3D10InputLayout* d_inputLayout;
    //! Variable to access current texture (actually shader resource view)
    ID3D10EffectShaderResourceVariable* d_boundTextureVariable;
    //! Variable to access world matrix used in geometry transformation.
    ID3D10EffectMatrixVariable* d_worldMatrixVariable;
    //! Variable to access projection matrix used in geometry transformation.
    ID3D10EffectMatrixVariable* d_projectionMatrixVariable;
};


} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif  // end of guard _CEGUIDirect3D10Renderer_h_
