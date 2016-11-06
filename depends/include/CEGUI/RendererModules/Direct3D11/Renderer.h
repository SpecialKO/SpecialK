/***********************************************************************
    created:    Wed May 5 2010
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
#ifndef _CEGUIDirect3D11Renderer_h_
#define _CEGUIDirect3D11Renderer_h_

#include "../../Renderer.h"
#include "../../Size.h"
#include "../../Vector.h"
#include <vector>
#include <map>

#if (defined( __WIN32__ ) || defined( _WIN32 )) && !defined(CEGUI_STATIC)
#   ifdef CEGUIDIRECT3D11RENDERER_EXPORTS
#       define D3D11_GUIRENDERER_API __declspec(dllexport)
#   else
#       define D3D11_GUIRENDERER_API __declspec(dllimport)
#   endif
#else
#   define D3D11_GUIRENDERER_API
#endif

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

// D3D forward refs
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3DX11Effect;//D3DXEffect11 in dependences
struct ID3DX11EffectTechnique;//D3DXEffect11 in dependences
struct ID3D11InputLayout;
struct ID3DX11EffectShaderResourceVariable;//D3DXEffect11 in dependences
struct ID3DX11EffectMatrixVariable;//D3DXEffect11 in dependences
struct ID3D11ShaderResourceView;//D3DXEffect11 in dependences
struct D3DXMATRIX;

#include <d3d11.h>
#include <d3dx11.h>
#include <d3dx10.h>


struct IDevice11//little structure that keeps both device, in order to reduce copy & paste around module
{
	//! The D3D device context we're using to create various resources with.
	ID3D11Device* d_device;
	//! The D3D device context we're using to render
	ID3D11DeviceContext* d_context;
};

// Start of CEGUI namespace section
namespace CEGUI
{
class Direct3D11GeometryBuffer;
class Direct3D11Texture;


//! Renderer implementation using Direct3D 10.
class D3D11_GUIRENDERER_API Direct3D11Renderer : public Renderer
{
public:
    /*!
    \brief
        Convenience function that creates the required objects to initialise the
        CEGUI system.

        This will create and initialise the following objects for you:
        - CEGUI::Direct3D11Renderer
        - CEGUI::DefaultResourceProvider
        - CEGUI::System

    \param device
        Pointer to the ID3D11Device interface that is to be used for CEGUI
        rendering operations.

    \param context
        Pointer to the ID3D11DeviceContext interface that is to be used for
        CEGUI rendering operations.

    \param abi
        This must be set to CEGUI_VERSION_ABI

    \return
        Reference to the CEGUI::Direct3D11Renderer object that was created.
    */
    static Direct3D11Renderer& bootstrapSystem(ID3D11Device* device,
                                               ID3D11DeviceContext* context,
                                               const int abi = CEGUI_VERSION_ABI);

    /*!
    \brief
        Convenience function to cleanup the CEGUI system and related objects
        that were created by calling the bootstrapSystem function.

        This function will destroy the following objects for you:
        - CEGUI::System
        - CEGUI::DefaultResourceProvider
        - CEGUI::Direct3D11Renderer

    \note
        If you did not initialise CEGUI by calling the bootstrapSystem function,
        you should \e not call this, but rather delete any objects you created
        manually.
    */
    static void destroySystem();

    /*!
    \brief
        Create an Direct3D11Renderer object.
    */
    static Direct3D11Renderer& create(ID3D11Device* device,ID3D11DeviceContext* context,
                                      const int abi = CEGUI_VERSION_ABI);

    /*!
    \brief
        Destroy an Direct3D11Renderer object.

    \param renderer
        The Direct3D11Renderer object to be destroyed.
    */
    static void destroy(Direct3D11Renderer& renderer);

//     //! return the ID3D10Device used by this renderer object.
//     ID3D11Device& getDirect3DDevice() const;
// 
// 	//! return the ID3D11Device context used by this renderer object.
// 	ID3D11DeviceContext& getDirect3DDeviceContext() const;

	//returns d3d11 container for further rendering and creating
	IDevice11& getDirect3DDevice(); 
    
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
	
    //! low-level function that binds the technique pass ready for use
    void bindTechniquePass(const BlendMode mode, const bool clipped);
    //! low-level function to set the texture shader resource view to be used.
    void setCurrentTextureShaderResource(ID3D11ShaderResourceView* srv); 
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
    Direct3D11Renderer(ID3D11Device* device,ID3D11DeviceContext* context);

    //! destructor.
    ~Direct3D11Renderer();

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

	IDevice11 d_device;

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
    typedef std::vector<Direct3D11GeometryBuffer*> GeometryBufferList;
    //! Container used to track geometry buffers.
    GeometryBufferList d_geometryBuffers;
    //! container type used to hold Textures we create.
    typedef std::map<String, Direct3D11Texture*, StringFastLessCompare
                     CEGUI_MAP_ALLOC(String, Direct3D11Texture*)> TextureMap;
    //! Container used to track textures.
    TextureMap d_textures;
    //! Effect (shader) used when rendering.
    ID3DX11Effect* d_effect;
    //! Rendering technique that supplies scissor clipped BM_NORMAL type rendering
    ID3DX11EffectTechnique* d_normalClippedTechnique;
    //! Rendering technique that supplies BM_NORMAL type rendering
    ID3DX11EffectTechnique* d_normalUnclippedTechnique;
    //! Rendering technique that supplies scissor clipped BM_RTT_PREMULTIPLIED type rendering
    ID3DX11EffectTechnique* d_premultipliedClippedTechnique;
    //! Rendering technique that supplies BM_RTT_PREMULTIPLIED type rendering
    ID3DX11EffectTechnique* d_premultipliedUnclippedTechnique;
    //! D3D11 input layout describing the vertex format we use.
    ID3D11InputLayout* d_inputLayout;
    //! Variable to access current texture (actually shader resource view)
    ID3DX11EffectShaderResourceVariable* d_boundTextureVariable;
    //! Variable to access world matrix used in geometry transformation.
    ID3DX11EffectMatrixVariable* d_worldMatrixVariable;
    //! Variable to access projection matrix used in geometry transformation.
    ID3DX11EffectMatrixVariable* d_projectionMatrixVariable;
};


} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif  // end of guard _CEGUIDirect3D11Renderer_h_
