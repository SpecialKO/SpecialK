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
#ifndef _CEGUIOpenGL3Shader_h_
#define _CEGUIOpenGL3Shader_h_

#include "CEGUI/Exceptions.h"
#include "RendererBase.h"
#include <string>

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

// Start of CEGUI namespace section
namespace CEGUI
{

class OPENGL_GUIRENDERER_API OpenGL3Shader :
    public AllocatedObject<OpenGL3Shader>
{
public:

    /*!
    \brief
        Creates and loads shader programs from the two strings supplied to it
    */
    OpenGL3Shader(const std::string& vertex_shader_source,
                  const std::string& fragment_shader_source);

    ~OpenGL3Shader();

    /*!
    \brief
        Bind the shader to the OGL state-machine
    */
    void bind() const;

    /*!
    \brief
        Unbind the shader
    */
    void unbind() const;

    /*!
    \brief
        Query the location of a vertex attribute inside the shader.
    */
    GLuint getAttribLocation(const std::string &name) const;

    /*!
    \brief
        Query the location of a uniform variable inside the shader.
    */
    GLuint getUniformLocation(const std::string &name) const;

    /*!
    \brief
        Defines the name of the variable inside the shader which represents the
        final color for each fragment.
    */
    void bindFragDataLocation(const std::string &name);

    bool isCreatedSuccessfully();

    void link();

private:
    GLuint compile(GLuint type, const std::string &source);

    void outputShaderLog(GLuint shader);
    void outputProgramLog(GLuint program);

    std::string d_shaderName;
    bool d_createdSucessfully;

    GLuint d_vertexShader;
    GLuint d_fragmentShader;
    GLuint d_geometryShader;
    GLuint d_program;
};

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define AT __FILE__ ":" TOSTRING(__LINE__)
#define checkGLErrors() getGLErrors(AT)

/*!
\brief
    Query OpenGL errors and process them in CEGUI
*/
void getGLErrors(const char *location);

}

#endif

