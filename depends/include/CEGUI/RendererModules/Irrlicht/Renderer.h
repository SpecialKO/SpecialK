/***********************************************************************
    created:    Tue Mar 3 2009
    author:     Paul D Turner (parts based on original code by Thomas Suter)
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
#ifndef _CEGUIIrrlichtRenderer_h_
#define _CEGUIIrrlichtRenderer_h_

#include "CEGUI/RendererModules/Irrlicht/RendererDef.h"
#include "../../Renderer.h"
#include "../../Size.h"
#include "../../Vector.h"

#include <vector>
#include <map>

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

// forward reference irrlicht classes
namespace irr
{
class IrrlichtDevice;
struct SEvent;

namespace video
{
class IVideoDriver;
}

namespace io
{
class IFileSystem;
}

}

// Start of CEGUI namespace section
namespace CEGUI
{
class IrrlichtTexture;
class IrrlichtGeometryBuffer;
class IrrlichtResourceProvider;
class IrrlichtEventPusher;
class IrrlichtImageCodec;

//! CEGUI::Renderer implementation for the Irrlicht engine.
class IRR_GUIRENDERER_API IrrlichtRenderer : public Renderer
{
public:
    /*!
    \brief
        Convenience function that creates all the Irrlicht specific objects and
        then initialises the CEGUI system with them.

        This will create and initialise the following objects for you:
        - CEGUI::IrrlichtRenderer
        - CEGUI::IrrlichtResourceProvider
        - CEGUI::IrrlichtImageCodec
        - CEGUI::System

    \param device
        Reference to the irr::IrrlichtDevice to be used when creating the
        CEGUI::IrrlichtRenderer object.

    \param abi
        This must be set to CEGUI_VERSION_ABI

    \return
        A reference to the CEGUI::IrrlichtRenderer object that was created.
    */
    static IrrlichtRenderer& bootstrapSystem(irr::IrrlichtDevice& device,
                                             const int abi = CEGUI_VERSION_ABI);

    /*!
    \brief
        Convenience function to cleanup the CEGUI system and related objects
        that were created by calling the bootstrapSystem function.

        This function will destroy the following objects for you:
        - CEGUI::System
        - CEGUI::IrrlichtImageCodec
        - CEGUI::IrrlichtResourceProvider
        - CEGUI::IrrlichtRenderer

    \note
        If you did not initialise CEGUI by calling the bootstrapSystem function,
        you should \e not call this, but rather delete any objects you created
        manually.
    */
    static void destroySystem();

    //! Function to create and return IrrlichtRenderer objects
    static IrrlichtRenderer& create(irr::IrrlichtDevice& device,
                                    const int abi = CEGUI_VERSION_ABI);

    //! Function to destroy IrrlichtRenderer objects.
    static void destroy(IrrlichtRenderer& renderer);

    //! Create a IrrlichtResourceProvider object.
    static IrrlichtResourceProvider&
        createIrrlichtResourceProvider(irr::io::IFileSystem& fs);

    //! Destroy a IrrlichtResourceProvider object.
    static void destroyIrrlichtResourceProvider(IrrlichtResourceProvider& rp);

    //! function to create a CEGUI::IrrlichtImageCodec object.
    static IrrlichtImageCodec& createIrrlichtImageCodec(
                                    irr::video::IVideoDriver& driver);

    //! function to destroy a CEGUI::IrrlichtImageCodec object.
    static void destroyIrrlichtImageCodec(IrrlichtImageCodec& ic);

    //! inject irrlicht event to CEGUI system
    bool injectEvent(const irr::SEvent& event);

    /*!
    \brief
        Helper to return a valid texture size according to device capabilities.

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
    bool isTexCoordSystemFlipped() const { return false; }

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
    const IrrlichtEventPusher* getEventPusher() const;

protected:
    //! Constructor
    IrrlichtRenderer(irr::IrrlichtDevice& device);
    //! Destructor
    ~IrrlichtRenderer();

    //! helper to throw exception if name is already used.
    void throwIfNameExists(const String& name) const;
    //! helper to safely log the creation of a named texture
    static void logTextureCreation(const String& name);
    //! helper to safely log the destruction of a named texture
    static void logTextureDestruction(const String& name);

    //! String holding the renderer identification text.
    static String d_rendererID;
    //! The IrrlichtDevide that we'll be using.
    irr::IrrlichtDevice& d_device;
    //! Irrlicht video driver (as obtained from the device)
    irr::video::IVideoDriver* d_driver;
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
    typedef std::vector<IrrlichtGeometryBuffer*> GeometryBufferList;
    //! Container used to track geometry buffers.
    GeometryBufferList d_geometryBuffers;
    //! container type used to hold Textures we create.
    typedef std::map<String, IrrlichtTexture*, StringFastLessCompare
                     CEGUI_MAP_ALLOC(String, IrrlichtTexture*)> TextureMap;
    //! Container used to track textures.
    TextureMap d_textures;
    //! What the renderer thinks the max texture size is.
    uint d_maxTextureSize;
    //! ptr to helper object that aids in injection of events from Irrlicht.
    IrrlichtEventPusher* d_eventPusher;
    //! true if driver supports non square textures
    bool d_supportsNSquareTextures;
    //! true if driver supports non power of two textures
    bool d_supportsNPOTTextures;
};

} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif  // end of guard _CEGUIIrrlichtRenderer_h_
