// ImGui GLFW binding with OpenGL3 + shaders
// In this binding, ImTextureID is used to store an OpenGL 'GLuint' texture identifier. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#include <SpecialK/stdafx.h>

#include <imgui/imgui.h>

#include <SpecialK/render/backend.h>
#include <../depends/include/GL/glew.h>
#include <SpecialK/render/gl/opengl_backend.h>

extern void
SK_ImGui_User_NewFrame (void);

// Data
static bool         g_MousePressed [3]       = { false, false, false };
static float        g_MouseWheel             = 0.0f;
static GLuint       g_FontTexture            = 0;
static int          g_ShaderHandle           = 0,
                    g_VertHandle             = 0,
                    g_FragHandle             = 0;
static int          g_AttribLocationTex      = 0,
                    g_AttribLocationProjMtx  = 0;
static int          g_AttribLocationPosition = 0,
                    g_AttribLocationUV       = 0,
                    g_AttribLocationColor    = 0;
static unsigned int g_VboHandle              = 0,
                    g_VaoHandle              = 0,
                    g_ElementsHandle         = 0;

#include <config.h>

// If text or lines are blurry when integrating ImGui in your engine:
// - in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
IMGUI_API
void
ImGui_ImplGL3_RenderDrawData (ImDrawData* draw_data)
{
  // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
  auto& io =
    ImGui::GetIO ();

  int fb_width  = static_cast <int> (io.DisplaySize.x);// * io.DisplayFramebufferScale.x);
  int fb_height = static_cast <int> (io.DisplaySize.y);// * io.DisplayFramebufferScale.y);

  if (fb_width == 0 || fb_height == 0)
    return;

  draw_data->ScaleClipRects (ImVec2 (1.0f, 1.0f));//io.DisplayFramebufferScale);

  // Backup GL state
  GLint     last_program;              glGetIntegerv (GL_CURRENT_PROGRAM,              &last_program);
  GLint     last_texture;              glGetIntegerv (GL_TEXTURE_BINDING_2D,           &last_texture);
  GLint     last_active_texture;       glGetIntegerv (GL_ACTIVE_TEXTURE,               &last_active_texture);
  GLint     last_array_buffer;         glGetIntegerv (GL_ARRAY_BUFFER_BINDING,         &last_array_buffer);
  GLint     last_element_array_buffer; glGetIntegerv (GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
  GLint     last_vertex_array;         glGetIntegerv (GL_VERTEX_ARRAY_BINDING,         &last_vertex_array);
  GLint     last_blend_src;            glGetIntegerv (GL_BLEND_SRC,                    &last_blend_src);
  GLint     last_blend_dst;            glGetIntegerv (GL_BLEND_DST,                    &last_blend_dst);
  GLint     last_blend_equation_rgb;   glGetIntegerv (GL_BLEND_EQUATION_RGB,           &last_blend_equation_rgb);
  GLint     last_blend_equation_alpha; glGetIntegerv (GL_BLEND_EQUATION_ALPHA,         &last_blend_equation_alpha);
  GLint     last_viewport    [4];      glGetIntegerv (GL_VIEWPORT,                      last_viewport);
  GLint     last_scissor_box [4];      glGetIntegerv (GL_SCISSOR_BOX,                   last_scissor_box);
  GLint     last_sampler;              glGetIntegerv (GL_SAMPLER_BINDING,              &last_sampler);
  GLboolean last_enable_blend        = glIsEnabled   (GL_BLEND);
  GLboolean last_enable_cull_face    = glIsEnabled   (GL_CULL_FACE);
  GLboolean last_enable_depth_test   = glIsEnabled   (GL_DEPTH_TEST);
  GLboolean last_enable_scissor_test = glIsEnabled   (GL_SCISSOR_TEST);
  GLboolean last_enable_stencil_test = glIsEnabled   (GL_STENCIL_TEST);
  GLboolean last_color_mask  [4];      glGetBooleanv (GL_COLOR_WRITEMASK,               last_color_mask);
  GLboolean last_depth_mask;           glGetBooleanv (GL_DEPTH_WRITEMASK,              &last_depth_mask);

  // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
  glEnable        (GL_BLEND);
  glBlendEquation (GL_FUNC_ADD);
  glBlendFunc     (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable       (GL_CULL_FACE);
  glDisable       (GL_DEPTH_TEST);
  glEnable        (GL_SCISSOR_TEST);
  glDisable       (GL_SCISSOR_TEST);
  glColorMask     (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glActiveTexture (GL_TEXTURE0);

  // Setup viewport, orthographic projection matrix
  glViewport ( 0, 0,
                 sk::narrow_cast <GLsizei> (fb_width),
                 sk::narrow_cast <GLsizei> (fb_height) );

  const float ortho_projection [4][4] =
  {
      { 2.0f/io.DisplaySize.x, 0.0f,                   0.0f, 0.0f },
      { 0.0f,                  2.0f/-io.DisplaySize.y, 0.0f, 0.0f },
      { 0.0f,                  0.0f,                  -1.0f, 0.0f },
      {-1.0f,                  1.0f,                   0.0f, 1.0f },
  };

  glUseProgram       (g_ShaderHandle);
  glUniform1i        (g_AttribLocationTex,     0);
  glUniformMatrix4fv (g_AttribLocationProjMtx, 1, GL_FALSE, &ortho_projection [0][0]);
  glBindVertexArray  (g_VaoHandle);

  // Will project scissor/clipping rectangles into framebuffer space
  ImVec2 clip_off   = draw_data->DisplayPos;       // (0,0) unless using multi-viewports
  ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

  for (int n = 0; n < draw_data->CmdListsCount; n++)
  {
    const ImDrawList* cmd_list =
      draw_data->CmdLists [n];

#if 0
    if (config.imgui.render.strip_alpha)
    {
      for (INT i = 0; i < cmd_list->VtxBuffer.Size; i++)
      {
#if 0 /// XXX:FIXME
        SK_ReleaseAssert (cmd_list->VtxBuffer.Data [i].col.x < 1.0 &&
                          cmd_list->VtxBuffer.Data [i].col.y < 1.0 &&
                          cmd_list->VtxBuffer.Data [i].col.z < 1.0 &&
                          cmd_list->VtxBuffer.Data [i].col.w < 1.0 );
#endif
        ImColor color (
          cmd_list->VtxBuffer.Data [i].col
        );

        uint8_t alpha = ((((unsigned int)color & 0xFF000000U) >> 24U) & 0xFFU);

        // Boost alpha for visibility
        if (alpha < 93 && alpha != 0)
          alpha += (93  - alpha) / 2;

        float a = ((float)                                     alpha / 255.0f);
        float r = ((float)(((unsigned int)color & 0xFF0000U) >> 16U) / 255.0f);
        float g = ((float)(((unsigned int)color & 0x00FF00U) >>  8U) / 255.0f);
        float b = ((float)(((unsigned int)color & 0x0000FFU)       ) / 255.0f);

        color =                    0xFF000000U  |
                ((UINT)((r * a) * 255U) << 16U) |
                ((UINT)((g * a) * 255U) <<  8U) |
                ((UINT)((b * a) * 255U)       );

        cmd_list->VtxBuffer.Data[i].col =
          color;
      }
    }
#endif

    glBindBuffer (GL_ARRAY_BUFFER,         g_VboHandle);
    glBufferData (GL_ARRAY_BUFFER,         (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof (ImDrawVert),
                                       (const GLvoid *)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);

    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, g_ElementsHandle);
    glBufferData (GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof (ImDrawIdx),
                                       (const GLvoid *)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

    for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
    {
      const ImDrawCmd* pcmd =
        &cmd_list->CmdBuffer [cmd_i];

      if (pcmd->UserCallback)
      {
        // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
        if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
          SK_ReleaseAssert (!L"ImDrawCallback_ResetRenderState = Not Implemented");// ImGui_ImplDX11_SetupRenderState (draw_data, pDevCtx);
        else
          pcmd->UserCallback (cmd_list, pcmd);
      }

      else
      {
        // Project scissor/clipping rectangles into framebuffer space
        ImVec2 clip_min ((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
        ImVec2 clip_max ((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);

        if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
            continue;

        glBindTexture  (GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->GetTexID ());
        glBindSampler  (0, 0);
        glScissor      ( (int) clip_min.x,               (int)((float)fb_height - clip_max.y),
                         (int)(clip_max.x - clip_min.x), (int)(      clip_max.y - clip_min.y) );

#ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_VTX_OFFSET
        if (bd->GlVersion >= 320)
            GL_CALL(glDrawElementsBaseVertex(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, (void*)(intptr_t)(pcmd->IdxOffset * sizeof(ImDrawIdx)), (GLint)pcmd->VtxOffset));
        else
#endif
        glDrawElements ( GL_TRIANGLES, (GLsizei)pcmd->ElemCount,
                         sizeof (ImDrawIdx) == 2 ?
                                   GL_UNSIGNED_SHORT :
                                   GL_UNSIGNED_INT,
                                     (void*)(intptr_t)(pcmd->IdxOffset * sizeof (ImDrawIdx)) );
      }
    }
  }

  // Restore modified GL state
  glUseProgram            (last_program);
  glActiveTexture         (last_active_texture);
  glBindTexture           (GL_TEXTURE_2D,           last_texture);
  glBindBuffer            (GL_ARRAY_BUFFER,         last_array_buffer);
  glBindBuffer            (GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
  glBindVertexArray       (last_vertex_array);
  glBlendEquationSeparate (last_blend_equation_rgb, last_blend_equation_alpha);
  glBlendFunc             (last_blend_src,          last_blend_dst);
  glColorMask             (last_color_mask [0],     last_color_mask [1],
                           last_color_mask [2],     last_color_mask [3]);

  if (! last_enable_blend)                                    glDisable (GL_BLEND);
  if (  last_enable_cull_face)    glEnable (GL_CULL_FACE);
  if (  last_enable_depth_test)   glEnable (GL_DEPTH_TEST);
  if (! last_enable_scissor_test)                             glDisable (GL_SCISSOR_TEST);
  if (  last_enable_stencil_test) glEnable (GL_SCISSOR_TEST);

  glViewport ( last_viewport    [0], last_viewport    [1], (GLsizei)last_viewport    [2], (GLsizei)last_viewport    [3]);
  glScissor  ( last_scissor_box [0], last_scissor_box [1], (GLsizei)last_scissor_box [2], (GLsizei)last_scissor_box [3]);
}

const char*
ImGui_ImplGL3_GetClipboardText (void* user_data)
{
  UNREFERENCED_PARAMETER (user_data);

  return nullptr;//return glfwGetClipboardString((GLFWwindow*)user_data);
}

void
ImGui_ImplGL3_SetClipboardText (void* user_data, const char* text)
{
  UNREFERENCED_PARAMETER (user_data);
  UNREFERENCED_PARAMETER (text);

  //glfwSetClipboardString((GLFWwindow*)user_data, text);
}

bool
ImGui_ImplGL3_CreateFontsTexture (void)
{
  // Build texture atlas
  ImGuiIO& io (
    ImGui::GetIO ()
  );

  unsigned char* pixels = nullptr;
  int            width  = 0,
                 height = 0;

  // Load as RGBA 32-bits (75% of the memory is wasted, but default font is so small)
  //   because it is more likely to be compatible with user's existing shaders.
  //
  //  If your ImTextureId represent a higher-level concept than just a GL texture id,
  //    consider calling GetTexDataAsAlpha8() instead to save on GPU memory.
  io.Fonts->GetTexDataAsRGBA32 (
    &pixels, &width,
             &height
  );

  // Upload texture to graphics system
  GLint last_texture;

  glGetIntegerv   ( GL_TEXTURE_BINDING_2D,               &last_texture  );
  glGenTextures   ( 1,                                   &g_FontTexture );
  glBindTexture   ( GL_TEXTURE_2D,                        g_FontTexture );
  glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                               GL_LINEAR                );
                                               //GL_LINEAR_MIPMAP_NEAREST );
  glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                               GL_LINEAR                );
  glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,  GL_CLAMP_TO_EDGE );
  glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,  GL_CLAMP_TO_EDGE );
  glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_R,  GL_CLAMP_TO_EDGE );

//glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD,                0 );
//glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD,               16 );
//glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL,             16 );
//glTexParameteri ( GL_TEXTURE_2D, GL_GENERATE_MIPMAP,          GL_TRUE );
  glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,     GL_NONE );


  GLint                                           last_unpack_buffer = 0;
  glGetIntegerv (GL_PIXEL_UNPACK_BUFFER_BINDING, &last_unpack_buffer);

  if (last_unpack_buffer != 0)
    glBindBuffer (GL_PIXEL_UNPACK_BUFFER, 0);

  //
  // Docs do not explain any attrib bits that backup pixel transfer,
  //   so do it ourselves...
  //
  GLboolean         UNPACK_SWAP_BYTES;
  glGetBooleanv (GL_UNPACK_SWAP_BYTES,   &UNPACK_SWAP_BYTES);
  glPixelStorei (GL_UNPACK_SWAP_BYTES,   false);
  GLboolean         UNPACK_LSB_FIRST;
  glGetBooleanv (GL_UNPACK_LSB_FIRST,	   &UNPACK_LSB_FIRST);
  glPixelStorei (GL_UNPACK_LSB_FIRST,    false);
  GLint             UNPACK_ROW_LENGTH;
  glGetIntegerv (GL_UNPACK_ROW_LENGTH,   &UNPACK_ROW_LENGTH);
  glPixelStorei (GL_UNPACK_ROW_LENGTH,       0);
  GLint             UNPACK_IMAGE_HEIGHT;
  glGetIntegerv (GL_UNPACK_IMAGE_HEIGHT, &UNPACK_IMAGE_HEIGHT);
  glPixelStorei (GL_UNPACK_IMAGE_HEIGHT,     0);
  GLint             UNPACK_SKIP_ROWS;
  glGetIntegerv (GL_UNPACK_SKIP_ROWS,    &UNPACK_SKIP_ROWS);
  glPixelStorei (GL_UNPACK_SKIP_ROWS,        0);
  GLint             UNPACK_SKIP_PIXELS;
  glGetIntegerv (GL_UNPACK_SKIP_PIXELS,  &UNPACK_SKIP_PIXELS);
  glPixelStorei (GL_UNPACK_SKIP_PIXELS,      0);
  GLint             UNPACK_SKIP_IMAGES;
  glGetIntegerv (GL_UNPACK_SKIP_IMAGES,  &UNPACK_SKIP_IMAGES);
  glPixelStorei (GL_UNPACK_SKIP_IMAGES,      0);
  GLint             UNPACK_ALIGNMENT;
  glGetIntegerv (GL_UNPACK_ALIGNMENT,    &UNPACK_ALIGNMENT);
  glPixelStorei (GL_UNPACK_ALIGNMENT,        4);

  glTexImage2D    ( GL_TEXTURE_2D, 0, GL_RGBA, width, height,         0,
                                      GL_RGBA, GL_UNSIGNED_BYTE, pixels );

  glPixelStorei (GL_UNPACK_SWAP_BYTES,    UNPACK_SWAP_BYTES);
  glPixelStorei (GL_UNPACK_LSB_FIRST,     UNPACK_LSB_FIRST);
  glPixelStorei (GL_UNPACK_ROW_LENGTH,    UNPACK_ROW_LENGTH);
  glPixelStorei (GL_UNPACK_IMAGE_HEIGHT,  UNPACK_IMAGE_HEIGHT);
  glPixelStorei (GL_UNPACK_SKIP_ROWS,     UNPACK_SKIP_ROWS);
  glPixelStorei (GL_UNPACK_SKIP_PIXELS,   UNPACK_SKIP_PIXELS);
  glPixelStorei (GL_UNPACK_SKIP_IMAGES,   UNPACK_SKIP_IMAGES);
  glPixelStorei (GL_UNPACK_ALIGNMENT,     UNPACK_ALIGNMENT);

  if (last_unpack_buffer != 0)
    glBindBuffer (GL_PIXEL_UNPACK_BUFFER, last_unpack_buffer);


  // Store our identifier
  io.Fonts->TexID =
    (void *)(intptr_t)g_FontTexture;

  // Restore state
  glBindTexture (GL_TEXTURE_2D, last_texture);

  return true;
}

bool
ImGui_ImplGlfwGL3_CreateDeviceObjects (void)
{
  // Backup GL state
  GLint last_texture,
        last_array_buffer,
        last_vertex_array;

  glGetIntegerv (GL_TEXTURE_BINDING_2D,   &last_texture     );
  glGetIntegerv (GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
  glGetIntegerv (GL_VERTEX_ARRAY_BINDING, &last_vertex_array);

  const GLchar *vertex_shader =
      "#version 330              \n"
      "                          \n"
      "uniform mat4 ProjMtx;     \n"
      "     in vec2 Position;    \n"
      "     in vec2 UV;          \n"
      "     in vec4 Color;       \n"
      "    out vec2 Frag_UV;     \n"
      "    out vec4 Frag_Color;  \n"
      "                          \n"
      "void main (void)          \n"
      "{                         \n"
      "  Frag_UV     = UV;       \n"
      "  Frag_Color  = Color;    \n"
      "  gl_Position = ProjMtx * \n"
      "    vec4 ( Position.xy,   \n"
      "                   0, 1 );\n"
      "}                         \n";

  const GLchar* fragment_shader =
      "#version 330                 \n"
      "                             \n"
      "uniform sampler2D Texture;   \n"
      "     in vec2      Frag_UV;   \n"
      "     in vec4      Frag_Color;\n"
      "    out vec4       Out_Color;\n"
      "                             \n"
      "void main (void)             \n"
      "{                            \n"
      "  Out_Color = Frag_Color *   \n"
      "    texture ( Texture,       \n"
      "              Frag_UV.st );  \n"
      "}                            \n";

  g_ShaderHandle = glCreateProgram (                  );
  g_VertHandle   = glCreateShader  ( GL_VERTEX_SHADER );
  g_FragHandle   = glCreateShader  (GL_FRAGMENT_SHADER);

  glShaderSource  (g_VertHandle, 1, &vertex_shader,   0);
  glShaderSource  (g_FragHandle, 1, &fragment_shader, 0);
  glCompileShader (g_VertHandle                        );
  glCompileShader (g_FragHandle                        );
  glAttachShader  (g_ShaderHandle, g_VertHandle        );
  glAttachShader  (g_ShaderHandle, g_FragHandle        );
  glLinkProgram   (g_ShaderHandle                      );

  g_AttribLocationTex      = glGetUniformLocation (g_ShaderHandle, "Texture" );
  g_AttribLocationProjMtx  = glGetUniformLocation (g_ShaderHandle, "ProjMtx" );
  g_AttribLocationPosition = glGetAttribLocation  (g_ShaderHandle, "Position");
  g_AttribLocationUV       = glGetAttribLocation  (g_ShaderHandle, "UV"      );
  g_AttribLocationColor    = glGetAttribLocation  (g_ShaderHandle, "Color"   );

  glGenBuffers      (1,              &g_VboHandle);
  glGenBuffers      (1,         &g_ElementsHandle);

  glGenVertexArrays (1,              &g_VaoHandle);
  glBindVertexArray (                 g_VaoHandle);
  glBindBuffer      (GL_ARRAY_BUFFER, g_VboHandle);

  glEnableVertexAttribArray (g_AttribLocationPosition);
  glEnableVertexAttribArray (g_AttribLocationUV      );
  glEnableVertexAttribArray (g_AttribLocationColor   );

#define OFFSETOF(TYPE, ELEMENT) ( reinterpret_cast <size_t> (     \
                                    &(reinterpret_cast <TYPE *> ( \
                                      nullptr                     \
                                     )->ELEMENT)                  \
                                  )                               \
                                )

  glVertexAttribPointer ( g_AttribLocationPosition,
                            2, GL_FLOAT,
                              GL_FALSE, sizeof (ImDrawVert),
  reinterpret_cast <GLvoid *> ( OFFSETOF       (ImDrawVert, pos) )
                        );
  glVertexAttribPointer ( g_AttribLocationUV,
                            2, GL_FLOAT,
                              GL_FALSE, sizeof (ImDrawVert),
  reinterpret_cast <GLvoid *> ( OFFSETOF       (ImDrawVert, uv) )
                        );
#if 0 // NO HDR ImGui Yet
  glVertexAttribPointer ( g_AttribLocationColor,
                            4, GL_FLOAT,
                               FALSE,  sizeof (ImDrawVert),
  reinterpret_cast <GLvoid *> ( OFFSETOF       (ImDrawVert, col) )
                        );
#else
  glVertexAttribPointer ( g_AttribLocationColor,
                            4, GL_UNSIGNED_BYTE,
                               TRUE,    sizeof (ImDrawVert),
  reinterpret_cast <GLvoid *> ( OFFSETOF       (ImDrawVert, col) )
                        );
#endif
#undef OFFSETOF

  ImGui_ImplGL3_CreateFontsTexture ();

  // Restore modified GL state
  glBindTexture     (GL_TEXTURE_2D,   last_texture     );
  glBindBuffer      (GL_ARRAY_BUFFER, last_array_buffer);
  glBindVertexArray (                 last_vertex_array);

  return true;
}

void
ImGui_ImplGL3_InvalidateDeviceObjects (void)
{
  extern void
  SK_ImGui_ResetExternal (void);
  SK_ImGui_ResetExternal (    );

  if (g_VaoHandle)      glDeleteVertexArrays (1, &g_VaoHandle);
  if (g_VboHandle)      glDeleteBuffers      (1, &g_VboHandle);
  if (g_ElementsHandle) glDeleteBuffers      (1, &g_ElementsHandle);

  g_VaoHandle = g_VboHandle = g_ElementsHandle = 0;

  if (g_ShaderHandle && g_VertHandle) glDetachShader (g_ShaderHandle, g_VertHandle);
  if (g_VertHandle)                   glDeleteShader (g_VertHandle);

  g_VertHandle = 0;

  if (g_ShaderHandle && g_FragHandle) glDetachShader (g_ShaderHandle, g_FragHandle);
  if (g_FragHandle)                   glDeleteShader (g_FragHandle);

  g_FragHandle = 0;

  if (g_ShaderHandle) glDeleteProgram (g_ShaderHandle);

  g_ShaderHandle = 0;

  if (g_FontTexture)
  {
    auto& io =
      ImGui::GetIO ();

    glDeleteTextures (
      1, &g_FontTexture
    );    g_FontTexture = 0;

    if (io.Fonts != nullptr)
        io.Fonts->TexID = nullptr;
  }
}

bool
ImGui_ImplGL3_Init (void)
{
  auto& io =
    ImGui::GetIO ();

  //io.ImeWindowHandle    = game_window.hWnd;
  //io.SetClipboardTextFn = ImGui_ImplGL3_SetClipboardText;
  //io.GetClipboardTextFn = ImGui_ImplGL3_GetClipboardText;
  io.ClipboardUserData  = game_window.hWnd;
  io.BackendFlags      &= ~ImGuiBackendFlags_RendererHasVtxOffset;

  return true;
}

void
ImGui_ImplGL3_Shutdown (void)
{
  ImGui_ImplGL3_InvalidateDeviceObjects ();
  ImGui::Shutdown ();
}

void
ImGui_ImplGL3_NewFrame (void)
{
  if (! g_FontTexture)
    ImGui_ImplGlfwGL3_CreateDeviceObjects ();

  auto& io =
    ImGui::GetIO ();

  ////game_window.hWnd =
  ////  WindowFromDC ( SK_GL_GetCurrentDC () );

  RECT                              client = { };
  GetClientRect (game_window.hWnd, &client);

  // Setup display size (every frame to accommodate for window resizing)
  int w = client.right  - client.left,
      h = client.bottom - client.top;

  io.DisplaySize             =
    ImVec2 ( static_cast <float> (w), static_cast <float> (h) );
  io.DisplayFramebufferScale =
    ImVec2 ( static_cast <float> (w), static_cast <float> (h) );


  // Stupid hack for engines that switch between shared contexts for alternate
  //   frames ---- for now, assume the framebuffer dimensions are the same.
  //
  static float last_fb_x = 1920.0f,
               last_fb_y = 1080.0f;

  if (io.DisplaySize.x <= 0.0f || io.DisplaySize.y <= 0.0f)
  {
    io.DisplaySize.x             = last_fb_x;
    io.DisplaySize.y             = last_fb_y;
    io.DisplayFramebufferScale.x = last_fb_x;
    io.DisplayFramebufferScale.y = last_fb_y;
  }

  else
  {
    last_fb_x = io.DisplaySize.x;
    last_fb_y = io.DisplaySize.y;
  }

  // Start the frame
  SK_ImGui_User_NewFrame ();
}