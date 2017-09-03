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

    std::wstring
    getPipelineStatsDesc (void);

  }
}


#include <cstdint>
#include <unordered_map>
#include <unordered_set>

enum
{
  Texture,
  Program,
  VertexBuffer,
};

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

#endif /* __SK__OPENGL_BACKEND_H__ */