/***********************************************************************
    created:    Wed, 8th Feb 2012
    author:     Lukas E Meindl
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2012 Paul D Turner & The CEGUI Development Team
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

#ifndef _CEGUIOpenGL3StandardShaderVert_h_
#define _CEGUIOpenGL3StandardShaderVert_h_

namespace CEGUI
{

const char
    StandardShaderVert_Opengl3[] =
        "#version 150 core\n"

        "uniform mat4 modelViewPerspMatrix;\n"
        "in vec3 inPosition;\n"
        "in vec2 inTexCoord;\n"
        "in vec4 inColour;\n"
        "out vec2 exTexCoord;\n"
        "out vec4 exColour;\n"

        "void main(void)\n"
        "{\n"
            "exTexCoord = inTexCoord;\n"
            "exColour = inColour;\n"
            "gl_Position = modelViewPerspMatrix * vec4(inPosition, 1.0);\n"
        "}",
    StandardShaderVert_OpenglEs2[] =
        "#version 100\n"
        "uniform mat4 modelViewPerspMatrix;\n"
        "attribute vec3 inPosition;\n"
        "attribute vec2 inTexCoord;\n"
        "attribute vec4 inColour;\n"
        "varying vec2 exTexCoord;\n"
        "varying vec4 exColour;\n"

        "void main(void)\n"
        "{\n"
            "exTexCoord = inTexCoord;\n"
            "exColour = inColour;\n"
            "gl_Position = modelViewPerspMatrix * vec4(inPosition, 1.0);\n"
        "}",
    StandardShaderVert_OpenglEs3[] =
        "#version 300 es\n"

        "uniform mat4 modelViewPerspMatrix;\n"
        "in vec3 inPosition;\n"
        "in vec2 inTexCoord;\n"
        "in vec4 inColour;\n"
        "out vec2 exTexCoord;\n"
        "out vec4 exColour;\n"

        "void main(void)\n"
        "{\n"
            "exTexCoord = inTexCoord;\n"
            "exColour = inColour;\n"
            "gl_Position = modelViewPerspMatrix * vec4(inPosition, 1.0);\n"
        "}";

}

#endif
