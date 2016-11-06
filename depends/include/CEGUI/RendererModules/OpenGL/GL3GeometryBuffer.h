/***********************************************************************
    created:    Wed, 8th Feb 2012
    author:     Lukas E Meindl (based on code by Paul D Turner)
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2013 Paul D Turner & The CEGUI Development Team
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
#ifndef _CEGUIOpenGL3GeometryBuffer_h_
#define _CEGUIOpenGL3GeometryBuffer_h_

#include "CEGUI/RendererModules/OpenGL/GeometryBufferBase.h"

namespace CEGUI
{
class OpenGL3Shader;
class OpenGL3StateChangeWrapper;
class OpenGL3Renderer;

//! OpenGL3 based implementation of the GeometryBuffer interface.
class OPENGL_GUIRENDERER_API OpenGL3GeometryBuffer : public OpenGLGeometryBufferBase
{
public:
    //! Constructor
    OpenGL3GeometryBuffer(OpenGL3Renderer& owner);
    virtual ~OpenGL3GeometryBuffer();

    void initialiseOpenGLBuffers();
    //! The functions first binds the vbo and then sets it up for rendering.
    void configureVertexArray() const;
    void deinitialiseOpenGLBuffers();
    void updateOpenGLBuffers();

    // implementation/overrides of members from GeometryBuffer
    void draw() const;
    void appendGeometry(const Vertex* const vbuff, uint vertex_count);
    void reset();

protected:
    //! OpenGL vao used for the vertices
    GLuint d_verticesVAO;
    //! OpenGL vbo containing all vertex data
    GLuint d_verticesVBO;
    //! Reference to the OpenGL shader inside the Renderer, that is used to render all geometry
    CEGUI::OpenGL3Shader*& d_shader;
    //! Position variable location inside the shader, for OpenGL
    const GLint d_shaderPosLoc;
    //! TexCoord variable location inside the shader, for OpenGL
    const GLint d_shaderTexCoordLoc;
    //! Color variable location inside the shader, for OpenGL
    const GLint d_shaderColourLoc;
    //! Matrix uniform location inside the shader, for OpenGL
    const GLint d_shaderStandardMatrixLoc;
    //! Pointer to the OpenGL state changer wrapper that was created inside the Renderer
    OpenGL3StateChangeWrapper* d_glStateChanger;
    //! Size of the buffer that is currently in use
    GLuint d_bufferSize;
};

}

#endif

