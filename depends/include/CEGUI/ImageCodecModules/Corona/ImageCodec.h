/***********************************************************************
	created:	07/06/2006
	author:		Olivier Delannoy 
	
	purpose:	This codec provide Corona based image loading 
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
#ifndef _CEGUICoronaImageCodec_h_
#define _CEGUICoronaImageCodec_h_
#include "../../ImageCodec.h"

#if (defined( __WIN32__ ) || defined( _WIN32 )) && !defined(CEGUI_STATIC)
#   ifdef CEGUICORONAIMAGECODEC_EXPORTS
#       define CEGUICORONAIMAGECODEC_API __declspec(dllexport)
#   else
#       define CEGUICORONAIMAGECODEC_API __declspec(dllimport)
#   endif
#else
#   define CEGUICORONAIMAGECODEC_API
#endif
// Start of CEGUI namespace section
namespace CEGUI
{
/*!
  \brief 
  Image codec based on the Corona library 
*/
class CEGUICORONAIMAGECODEC_API CoronaImageCodec : public ImageCodec 
{
public:
    CoronaImageCodec();
    ~CoronaImageCodec();
    
    Texture* load(const RawDataContainer& data, Texture* result);
protected:

private:

};    

} // End of CEGUI namespace section 

#endif // end of guard _CEGUICoronaImageCodec_h_
