/**
 * This file is part of Special K.
 *
 * Special K is free software : you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by The Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * Special K is distributed in the hope that it will be useful,
 *
 * But WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Special K.
 *
 *   If not, see <http://www.gnu.org/licenses/>.
 *
**/

#ifndef __SK__OPENGL_BACKEND_H__
#define __SK__OPENGL_BACKEND_H__

#include <string>

namespace SK
{
  namespace OpenGL
  {

    bool Startup  (void);
    bool Shutdown (void);

    std::string
    getPipelineStatsDesc (void);

  }
}


#include <cstdint>
#include <unordered_map>
#include <unordered_set>

//enum
//{
//  Texture,
//  Program,
//  VertexBuffer,
//};

void
SK_GL_PushMostStates (void);

void
SK_GL_PopMostStates (void);

#if 0
struct SK_GL_KnownShaders
{
  //typedef std::unordered_map <uint32_t, std::unordered_set <ID3D11ShaderResourceView *>> conditional_blacklist_t;

  template <typename _T>
  struct ShaderRegistry
  {
    void clear (void)
    {
      clear_state ( );

      changes_last_frame = 0;
    }

    void clear_state (void)
    {
      //current.crc32c = 0x0;
      //current.ptr = nullptr;
    }

    std::unordered_map <_T*, uint32_t>      rev;
    //std::unordered_map <uint32_t, SK_D3D11_ShaderDesc> descs;

    std::unordered_set <uint32_t>           wireframe;
    std::unordered_set <uint32_t>           blacklist;

    //conditional_blacklist_t               blacklist_if_texture;

    struct
    {
      uint32_t                              crc32c = 0x00;
      _T*                                   ptr = nullptr;
    } current;

    //gl_shader_tracking_s                  tracked;

    ULONG                                   changes_last_frame = 0;
  };

  ShaderRegistry <GLuint> pixel;
  ShaderRegistry <GLuint> vertex;
  ShaderRegistry <GLuint> geometry;
  ShaderRegistry <GLuint> tess_control;
  ShaderRegistry <GLuint> tess_eval;
  ShaderRegistry <GLuint> compute;
} extern SK_GL_Programs;

struct SK_GL_KnownTextures
{
  struct TextureRegistry
  {
    void clear (void)
    {
      rev.clear ( );
      clear_state ( );

      changes_last_frame = 0;
    }

    void clear_state (void)
    {
      current.crc32c = 0x0;
      current.ptr = nullptr;
    }

    std::unordered_map <_T*, uint32_t>      rev;
    //std::unordered_map <uint32_t, SK_D3D11_ShaderDesc> descs;

    std::unordered_set <uint32_t>           wireframe;
    std::unordered_set <uint32_t>           blacklist;

    //conditional_blacklist_t               blacklist_if_texture;

    struct
    {
      uint32_t                              crc32c = 0x00;
      _T*                                   ptr = nullptr;
    } current;

    //gl_shader_tracking_s                  tracked;
  }

  ULONG                                   changes_last_frame = 0;
  TextureRegistry <GLuint>  tex2D;

struct SK_GL_KnownTextures
{
} extern SK_GL_Textures;

struct SK_GL_KnownBuffers
{ } extern SK_GL_Buffers;

struct SK_GL_KnownArrayObjects
{ } extern SK_GL_Arrays;

struct SK_GL_KnownArrayObjects
{ } extern SK_GL_Arrays;
#endif

void
WINAPI
SK_HookGL (void);

extern HGLRC WINAPI SK_GL_GetCurrentContext (void);
extern HDC   WINAPI SK_GL_GetCurrentDC      (void);

extern BOOL  WINAPI SK_GL_SwapInterval      (int interval);
extern int   WINAPI SK_GL_GetSwapInterval   (void);

#ifndef GL_VERSION_1_1
typedef unsigned int GLenum;
typedef int GLint;
#endif

struct SK_GL_ClipControl {
#define GL_CLIP_ORIGIN         0x935C
#define GL_CLIP_DEPTH_MODE     0x935D

#define GL_NEGATIVE_ONE_TO_ONE 0x935E
#define GL_ZERO_TO_ONE         0x935F

#define GL_LOWER_LEFT          0x8CA1
#define GL_UPPER_LEFT          0x8CA2

using  glClipControl_pfn = void (WINAPI *)(GLenum origin, GLenum depth);
static glClipControl_pfn
       glClipControl;

  struct {
    GLint origin     = GL_LOWER_LEFT;
    GLint depth_mode = GL_NEGATIVE_ONE_TO_ONE;
  } original;

  void push (GLint origin, GLint depth_mode);
  void pop  (void);
} extern glClipControlARB;

#endif /* __SK__OPENGL_BACKEND_H__ */