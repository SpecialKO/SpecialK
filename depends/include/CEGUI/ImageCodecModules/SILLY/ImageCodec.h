/***********************************************************************
	created:	Thu Jun 15 2006
	author:		Tomas Lindquist Olsen

	purpose:	This codec provides SILLY based image loading
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
#ifndef _CEGUISILLYImageCodec_h_
#define _CEGUISILLYImageCodec_h_
#include "../../ImageCodec.h"


#if (defined( __WIN32__ ) || defined( _WIN32 )) && !defined(CEGUI_STATIC)
#   ifdef CEGUISILLYIMAGECODEC_EXPORTS
#       define CEGUISILLYIMAGECODEC_API __declspec(dllexport)
#   else
#       define CEGUISILLYIMAGECODEC_API __declspec(dllimport)
#   endif
#else
#   define CEGUISILLYIMAGECODEC_API
#endif


// Start of CEGUI namespace section
namespace CEGUI
{
/*!
\brief
    Image codec based on the SILLY library.
*/
class CEGUISILLYIMAGECODEC_API SILLYImageCodec : public ImageCodec 
{
public:
    SILLYImageCodec();
    ~SILLYImageCodec();

    Texture* load(const RawDataContainer& data, Texture* result);
};    

} // End of CEGUI namespace section 

#endif // end of guard _CEGUISILLYImageCodec_h_
