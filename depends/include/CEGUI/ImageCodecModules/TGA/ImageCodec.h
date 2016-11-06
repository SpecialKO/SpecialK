/***********************************************************************
	created:	03/06/2006
	author:		Olivier Delannoy 
	
	purpose:	This codec provide TGA image loading, it's the default 
                codec 
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2006 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUITGAImageCodec_h_
#define _CEGUITGAImageCodec_h_
#include "../../ImageCodec.h"

#if (defined( __WIN32__ ) || defined( _WIN32 )) && !defined(CEGUI_STATIC)
#   ifdef CEGUITGAIMAGECODEC_EXPORTS
#       define CEGUITGAIMAGECODEC_API __declspec(dllexport)
#   else
#       define CEGUITGAIMAGECODEC_API __declspec(dllimport)
#   endif
#else
#   define CEGUITGAIMAGECODEC_API
#endif

namespace CEGUI 
{
/*!
  \brief 
  Default image codec 

  This image codec is able to load TGA file only. 
  it is always available. 
*/
class CEGUITGAIMAGECODEC_API TGAImageCodec : public ImageCodec
{
    /*! 
      \brief 
      This is our image structure for our targa data
    */
    struct ImageTGA
    {
        int channels;        //!< The channels in the image (3 = RGB : 4 = RGBA)
        int sizeX;           //!< The width of the image in pixels
        int sizeY;           //!< The height of the image in pixels
        unsigned char *data; //!< The image pixel data
    };
    
    //! vertically flip data for tImageTGA 'img'
    static void flipVertImageTGA(ImageTGA* img);
    //! horizontally flip data for tImageTGA 'img'
    static void flipHorzImageTGA(ImageTGA* img);

    /*!
      \brief 
      load a TGA from a byte buffer 
    */
    static ImageTGA* loadTGA(const unsigned char* buffer, size_t buffer_size);
    /*!
      \brief 
      convert 24 bits Image to 32 bits one 
    */
    static void convertRGBToRGBA(ImageTGA* img);
        
public:
    TGAImageCodec();

    ~TGAImageCodec();
    
    // Took this code from http://www.gametutorials.com still ne
    // tImageTGA *LoadTGA(const char *filename)
    //
    // This is our cool function that loads the targa (TGA) file, then returns it's data.  
    // This tutorial supports 16, 24 and 32 bit images, along with RLE compression.
    //
    //
    // Ben Humphrey (DigiBen)
    // Game Programmer
    // DigiBen@GameTutorials.com
    // Co-Web Host of www.GameTutorials.com
    Texture* load(const RawDataContainer& data, Texture* result);

protected:
private:
};

} // End of CEGUI namespace section 

#endif // end of guard _CEGUITGAImageCodec_h_
