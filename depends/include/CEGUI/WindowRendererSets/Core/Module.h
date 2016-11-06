/***********************************************************************
    created:    Wed Jun 22 2005
    author:     Paul D Turner <paul@cegui.org.uk>
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2009 Paul D Turner & The CEGUI Development Team
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
#ifndef _FalModule_h_
#define _FalModule_h_

#include "CEGUI/FactoryModule.h"
#include "CEGUI/WindowRenderer.h"

#if (defined( __WIN32__ ) || defined( _WIN32 )) && !defined(CEGUI_STATIC)
#   ifdef CEGUICOREWINDOWRENDERERSET_EXPORTS
#       define COREWRSET_API __declspec(dllexport)
#   else
#       define COREWRSET_API __declspec(dllimport)
#   endif
#else
#   define COREWRSET_API
#endif

//! Function we're required to export.
extern "C"
COREWRSET_API CEGUI::FactoryModule& getWindowRendererFactoryModule();

namespace CEGUI
{
//! Implementation of WindowRendererModule for the Falagard window renderers
class CoreWindowRendererModule : public CEGUI::FactoryModule
{
public:
    CoreWindowRendererModule();
    ~CoreWindowRendererModule();
};

}

#endif  // end of guard _FalModule_h_
