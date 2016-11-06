/***********************************************************************
	created:	04/07/2014
	author:		Luca Ebach <bitbucket@lucebac.net>
                        with code by John Norman
	
	purpose:	This codec provides SDL2 based image loading 
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2015 Paul D Turner & The CEGUI Development Team
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

#ifndef _SDL2_IMAGE_CODEC_H_
#define _SDL2_IMAGE_CODEC_H_

#if (defined( __WIN32__ ) || defined( _WIN32 )) && !defined(CEGUI_STATIC)
#   ifdef CEGUISDL2IMAGECODEC_EXPORTS
#       define CEGUISDL2IMAGECODEC_API __declspec(dllexport)
#   else
#       define CEGUISDL2IMAGECODEC_API __declspec(dllimport)
#   endif
#else
#   define CEGUISDL2IMAGECODEC_API
#endif

#include <CEGUI/ImageCodec.h>
#include <CEGUI/Texture.h>

#include <SDL2/SDL_video.h>

namespace CEGUI
{

class CEGUISDL2IMAGECODEC_API SDL2ImageCodec : public ImageCodec 
{
public:
    SDL2ImageCodec();
    ~SDL2ImageCodec();
    
    Texture* load(const RawDataContainer& data, Texture* result);
};

} // End namespace 

#endif	/* IMAGECODEC_H */

