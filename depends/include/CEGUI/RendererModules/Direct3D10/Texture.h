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
#ifndef _CEGUIDirect3D10Texture_h_
#define _CEGUIDirect3D10Texture_h_

#include "../../Texture.h"
#include "CEGUI/RendererModules/Direct3D10/Renderer.h"
#include "../../Size.h"
#include "../../Vector.h"

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

// d3d forward refs
struct ID3D10Device;
struct ID3D10Texture2D;
struct ID3D10ShaderResourceView;

// Start of CEGUI namespace section
namespace CEGUI
{
//! Texture implementation for the Direct3D10Renderer.
class D3D10_GUIRENDERER_API Direct3D10Texture : public Texture
{
public:
    /*!
    \brief
        set the D3D10 texture that this Texture is based on to the specified
        texture.
    */
    void setDirect3DTexture(ID3D10Texture2D* tex);

    /*!
    \brief
        Return the internal D3D10 texture used by this Texture object.

    \return
        Pointer to the D3D10 texture interface that this object is using.
    */
    ID3D10Texture2D* getDirect3DTexture() const;

    /*!
    \brief
        Return the internal D3D10 shader resource view for the texture.

    \return
        Pointer to the ID3D10ShaderResourceView interface.
    */
    ID3D10ShaderResourceView* getDirect3DShaderResourceView() const;

    /*!
    \brief
        Sets what the texture should consider as the original data size.

    \note
        This also causes the texel scaling values to be updated.
    */
    void setOriginalDataSize(const Sizef& sz);

    // implement abstract members from base class.
    const String& getName() const;
    const Sizef& getSize() const;
    const Sizef& getOriginalDataSize() const;
    const Vector2f& getTexelScaling() const;
    void loadFromFile(const String& filename, const String& resourceGroup);
    void loadFromMemory(const void* buffer, const Sizef& buffer_size,
                        PixelFormat pixel_format);
    void blitFromMemory(const void* sourceData, const Rectf& area);
    void blitToMemory(void* targetData);
    bool isPixelFormatSupported(const PixelFormat fmt) const;

protected:
    // Friends to allow Renderer to peform construction and destruction
    friend Texture& Direct3D10Renderer::createTexture(const String&);
    friend Texture& Direct3D10Renderer::createTexture(const String&,
                                                      const String&,
                                                      const String&);
    friend Texture& Direct3D10Renderer::createTexture(const String&,
                                                      const Sizef&);
    //friend Texture& Direct3D10Renderer::createTexture(ID3D10Texture2D* tex);
    friend void Direct3D10Renderer::destroyTexture(Texture&);
    friend void Direct3D10Renderer::destroyTexture(const String&);

    //! Basic constructor.
    Direct3D10Texture(ID3D10Device& device, const String&);
    //! Construct texture from an image file.
    Direct3D10Texture(ID3D10Device& device, const String&,
                      const String& filename, const String& resourceGroup);
    //! Construct texture with a given size.
    Direct3D10Texture(ID3D10Device& device, const String&, const Sizef& sz);
    //! Construct texture that wraps an existing D3D10 texture.
    Direct3D10Texture(ID3D10Device& device, const String&, ID3D10Texture2D* tex);
    //! Destructor.
    virtual ~Direct3D10Texture();

    //! clean up the internal texture.
    void cleanupDirect3D10Texture();
    //! updates cached scale value used to map pixels to texture co-ords.
    void updateCachedScaleValues();
    //! set d_size to actual texture size (d_dataSize is used if query fails)
    void updateTextureSize();
    //! creates shader resource view for the current D3D texture
    void initialiseShaderResourceView();

    //! D3D device used to do the business.
    ID3D10Device& d_device;
    //! The D3D 10 texture we're wrapping.
    ID3D10Texture2D* d_texture;
    //! Shader resource view for the texture.
    ID3D10ShaderResourceView* d_resourceView;
    //! Size of the texture.
    Sizef d_size;
    //! original pixel of size data loaded into texture
    Sizef d_dataSize;
    //! cached pixel to texel mapping scale values.
    Vector2f d_texelScaling;
    //! The name used when the texture was created.
    const String d_name;
};


} // End of  CEGUI namespace section

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif  // end of guard _CEGUIDirect3D10Texture_h_
