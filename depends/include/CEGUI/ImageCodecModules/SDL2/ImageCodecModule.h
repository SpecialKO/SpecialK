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

#ifndef _SDL2_IMAGE_CODEC_MODULE_H_
#define _SDL2_IMAGE_CODEC_MODULE_H_

#include <CEGUI/ImageCodecModules/SDL2/ImageCodec.h>


/*! 
  \brief 
  exported function that creates the ImageCodec based object and 
  returns a pointer to that object.
*/
extern "C" CEGUISDL2IMAGECODEC_API CEGUI::ImageCodec* createImageCodec(void);

/*!
  \brief
  exported function that deletes an ImageCodec based object previously 
  created by this module.
*/
extern "C" CEGUISDL2IMAGECODEC_API void destroyImageCodec(CEGUI::ImageCodec* imageCodec);


#endif	/* _SDL2_IMAGE_CODEC_MODULE_H_ */

