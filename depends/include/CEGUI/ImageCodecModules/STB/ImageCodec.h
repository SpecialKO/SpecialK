/***********************************************************************
	created:	Fri Apr 30 2010
	author:		Tobias Schlegel

	purpose:	This codec provides stb_image.c based image loading
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
#ifndef _CEGUISTBImageCodec_h_
#define _CEGUISTBImageCodec_h_
#include "../../ImageCodec.h"


#if (defined( __WIN32__ ) || defined( _WIN32 )) && !defined(CEGUI_STATIC)
#   ifdef CEGUISTBIMAGECODEC_EXPORTS
#       define CEGUISTBIMAGECODEC_API __declspec(dllexport)
#   else
#       define CEGUISTBIMAGECODEC_API __declspec(dllimport)
#   endif
#else
#   define CEGUISTBIMAGECODEC_API
#endif


// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    Image codec based on stb_image.c.
*/
class CEGUISTBIMAGECODEC_API STBImageCodec : public ImageCodec 
{
public:
    STBImageCodec();
    ~STBImageCodec();

    Texture* load(const RawDataContainer& data, Texture* result);
};    

} // End of CEGUI namespace section 

#endif // end of guard _CEGUISTBImageCodec_h_

