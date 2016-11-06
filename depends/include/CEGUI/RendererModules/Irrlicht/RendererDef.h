/***********************************************************************
    created:    20/7/2004
    author:     Thomas Suter

    changes:
        - Irrlicht patching not needed anymore
        - using the irrlicht filesystem to load config files etc.
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

/*
 Beginning from version 0.7 Irrlicht does NOT need any changes for the GUI-renderer.
 Thanks to Nikolaus Gebhardt for including the missing methods in the renderer.
*/

#ifndef IRRLICHTRENDERERDEF_H_INCLUDED
#define IRRLICHTRENDERERDEF_H_INCLUDED

#if (defined( __WIN32__ ) || defined( _WIN32 ) || defined (WIN32)) && !defined(CEGUI_STATIC)
#   ifdef CEGUIIRRLICHTRENDERER_EXPORTS
#       define IRR_GUIRENDERER_API __declspec(dllexport)
#   else
#       define IRR_GUIRENDERER_API __declspec(dllimport)
#   endif
#else
#   define IRR_GUIRENDERER_API
#endif

#endif
