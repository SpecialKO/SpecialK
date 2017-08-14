// ImGui GLFW binding with OpenGL3 + shaders
// In this binding, ImTextureID is used to store an OpenGL 'GLuint' texture identifier. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#include <imgui/imgui.h>

#include <Windows.h>
#include <../depends/include/GL/glew.h>
#include <SpecialK/framerate.h>

#include <SpecialK/window.h>

// Data
static INT64        g_Time                   = 0;
static INT64        g_TicksPerSecond         = 0;
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

// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
// If text or lines are blurry when integrating ImGui in your engine:
// - in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
IMGUI_API
void
ImGui_ImplGL3_RenderDrawLists (ImDrawData* draw_data)
{
  // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
  ImGuiIO& io (ImGui::GetIO ());

  int fb_width  = static_cast <int> (io.DisplaySize.x);// * io.DisplayFramebufferScale.x);
  int fb_height = static_cast <int> (io.DisplaySize.y);// * io.DisplayFramebufferScale.y);

  if (fb_width == 0 || fb_height == 0)
    return;

  draw_data->ScaleClipRects (ImVec2 (1.0f, 1.0f));//io.DisplayFramebufferScale);

  //
  // nb: State bckup/restore is handled higher up the callstack
  //
#if 0
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
  GLboolean last_enable_blend        = glIsEnabled   (GL_BLEND);
  GLboolean last_enable_cull_face    = glIsEnabled   (GL_CULL_FACE);
  GLboolean last_enable_depth_test   = glIsEnabled   (GL_DEPTH_TEST);
  GLboolean last_enable_scissor_test = glIsEnabled   (GL_SCISSOR_TEST);
#endif

  // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
  glEnable        (GL_BLEND);
  glBlendEquation (GL_FUNC_ADD);
  glBlendFunc     (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable       (GL_CULL_FACE);
  glDisable       (GL_DEPTH_TEST);
  glEnable        (GL_SCISSOR_TEST);
  glActiveTexture (GL_TEXTURE0);

  // Setup viewport, orthographic projection matrix
  glViewport ( 0, 0,
                 static_cast <GLsizei> (fb_width),
                 static_cast <GLsizei> (fb_height) );

  const float ortho_projection [4][4] =
  {
      { 2.0f/io.DisplaySize.x, 0.0f,                   0.0f, 0.0f },
      { 0.0f,                  2.0f/-io.DisplaySize.y, 0.0f, 0.0f },
      { 0.0f,                  0.0f,                  -1.0f, 0.0f },
      {-1.0f,                  1.0f,                   0.0f, 1.0f },
  };

  glUseProgram       (g_ShaderHandle);
  glUniform1i        (g_AttribLocationTex,     0);
  glUniformMatrix4fv (g_AttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);
  glBindVertexArray  (g_VaoHandle);

  for (int n = 0; n < draw_data->CmdListsCount; n++)
  {
    const ImDrawList* cmd_list          = draw_data->CmdLists [n];
    const ImDrawIdx*  idx_buffer_offset = 0;

    glBindBuffer (GL_ARRAY_BUFFER,         g_VboHandle);
    glBufferData (GL_ARRAY_BUFFER,         (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof (ImDrawVert),
                                       (const GLvoid *)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);

    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, g_ElementsHandle);
    glBufferData (GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof (ImDrawIdx),
                                       (const GLvoid *)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

    for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
    {
      const ImDrawCmd* pcmd = &cmd_list->CmdBuffer [cmd_i];
      if (pcmd->UserCallback)
      {
        pcmd->UserCallback (cmd_list, pcmd);
      }
      else
      {
        glBindTexture  (GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
        glScissor      ( (int) pcmd->ClipRect.x,                     (int)(fb_height        - pcmd->ClipRect.w),
                         (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y) );
        glDrawElements ( GL_TRIANGLES, (GLsizei)pcmd->ElemCount,
                           sizeof (ImDrawIdx) == 2 ?
                                     GL_UNSIGNED_SHORT :
                                     GL_UNSIGNED_INT,
                                       idx_buffer_offset );
      }
      idx_buffer_offset += pcmd->ElemCount;
    }
  }

#if 0
  // Restore modified GL state
  glUseProgram            (last_program);
  glActiveTexture         (last_active_texture);
  glBindTexture           (GL_TEXTURE_2D,           last_texture);
  glBindVertexArray       (last_vertex_array);
  glBlendEquationSeparate (last_blend_equation_rgb, last_blend_equation_alpha);
  glBlendFunc             (last_blend_src,          last_blend_dst);

  if (last_enable_blend)        glEnable (GL_BLEND);        else glDisable (GL_BLEND);
  if (last_enable_cull_face)    glEnable (GL_CULL_FACE);    else glDisable (GL_CULL_FACE);
  if (last_enable_depth_test)   glEnable (GL_DEPTH_TEST);   else glDisable (GL_DEPTH_TEST);
  if (last_enable_scissor_test) glEnable (GL_SCISSOR_TEST); else glDisable (GL_SCISSOR_TEST);

  glViewport ( last_viewport    [0], last_viewport    [1], (GLsizei)last_viewport    [2], (GLsizei)last_viewport    [3]);
  glScissor  ( last_scissor_box [0], last_scissor_box [1], (GLsizei)last_scissor_box [2], (GLsizei)last_scissor_box [3]);
#endif
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
  ImGuiIO& io (ImGui::GetIO ());

  extern void
  SK_ImGui_LoadFonts (void);

  SK_ImGui_LoadFonts ();

  unsigned char* pixels = nullptr;
  int            width  = 0,
                 height = 0;

  // Load as RGBA 32-bits (75% of the memory is wasted, but default font is so small)
  //   because it is more likely to be compatible with user's existing shaders.
  //
  //  If your ImTextureId represent a higher-level concept than just a GL texture id,
  //    consider calling GetTexDataAsAlpha8() instead to save on GPU memory.
  io.Fonts->GetTexDataAsRGBA32 (&pixels, &width, &height);
  
  // Upload texture to graphics system
  GLint last_texture;

  glGetIntegerv   ( GL_TEXTURE_BINDING_2D, &last_texture               );
  glGenTextures   ( 1,                     &g_FontTexture              );
  glBindTexture   ( GL_TEXTURE_2D,          g_FontTexture              );
  glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR    );
  glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR    );
  glTexImage2D    ( GL_TEXTURE_2D, 0, GL_RGBA, width, height,    0,
                                     GL_RGBA, GL_UNSIGNED_BYTE, pixels );
  
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
      "#version 330\n"
      "uniform mat4 ProjMtx;\n"
      "in vec2 Position;\n"
      "in vec2 UV;\n"
      "in vec4 Color;\n"
      "out vec2 Frag_UV;\n"
      "out vec4 Frag_Color;\n"
      "void main()\n"
      "{\n"
      "	Frag_UV = UV;\n"
      "	Frag_Color = Color;\n"
      "	gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
      "}\n";

  const GLchar* fragment_shader =
      "#version 330\n"
      "uniform sampler2D Texture;\n"
      "in vec2 Frag_UV;\n"
      "in vec4 Frag_Color;\n"
      "out vec4 Out_Color;\n"
      "void main()\n"
      "{\n"
      "	Out_Color = Frag_Color * texture( Texture, Frag_UV.st);\n"
      "}\n";

  g_ShaderHandle = glCreateProgram (                  );
  g_VertHandle   = glCreateShader  ( GL_VERTEX_SHADER );
  g_FragHandle   = glCreateShader  (GL_FRAGMENT_SHADER);

  glShaderSource  (g_VertHandle, 1, &vertex_shader,   0);
  glShaderSource  (g_FragHandle, 1, &fragment_shader, 0);
  glCompileShader (g_VertHandle);
  glCompileShader (g_FragHandle);
  glAttachShader  (g_ShaderHandle, g_VertHandle);
  glAttachShader  (g_ShaderHandle, g_FragHandle);
  glLinkProgram   (g_ShaderHandle);

  g_AttribLocationTex      = glGetUniformLocation (g_ShaderHandle, "Texture" );
  g_AttribLocationProjMtx  = glGetUniformLocation (g_ShaderHandle, "ProjMtx" );
  g_AttribLocationPosition = glGetAttribLocation  (g_ShaderHandle, "Position");
  g_AttribLocationUV       = glGetAttribLocation  (g_ShaderHandle, "UV"      );
  g_AttribLocationColor    = glGetAttribLocation  (g_ShaderHandle, "Color"   );

  glGenBuffers (1, &g_VboHandle);
  glGenBuffers (1, &g_ElementsHandle);

  glGenVertexArrays (1,              &g_VaoHandle);
  glBindVertexArray (                 g_VaoHandle);
  glBindBuffer      (GL_ARRAY_BUFFER, g_VboHandle);

  glEnableVertexAttribArray (g_AttribLocationPosition);
  glEnableVertexAttribArray (g_AttribLocationUV);
  glEnableVertexAttribArray (g_AttribLocationColor);

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
  glVertexAttribPointer ( g_AttribLocationColor,
                            4, GL_UNSIGNED_BYTE,
                              GL_TRUE,  sizeof (ImDrawVert),
  reinterpret_cast <GLvoid *> ( OFFSETOF       (ImDrawVert, col) )
                        );
#undef OFFSETOF

  ImGui_ImplGL3_CreateFontsTexture ();

  // Restore modified GL state
  glBindTexture     (GL_TEXTURE_2D,   last_texture);
  glBindBuffer      (GL_ARRAY_BUFFER, last_array_buffer);
  glBindVertexArray (last_vertex_array);

  return true;
}

void
ImGui_ImplGL3_InvalidateDeviceObjects (void)
{
  extern void
  SK_ImGui_ResetExternal (void);
  SK_ImGui_ResetExternal ();

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
    ImGuiIO& io (ImGui::GetIO ());

    glDeleteTextures (1, &g_FontTexture);
    io.Fonts->TexID = 0;
    g_FontTexture   = 0;
  }
}

bool
ImGui_ImplGL3_Init (void)
{
  static bool first = true;

  if (first) {
    if (! QueryPerformanceFrequency        (reinterpret_cast <LARGE_INTEGER *> (&g_TicksPerSecond)))
      return false;

    if (! QueryPerformanceCounter_Original (reinterpret_cast <LARGE_INTEGER *> (&g_Time)))
      return false;

    first = false;
  }

  //g_Window = window;

  ImGuiIO& io (ImGui::GetIO ());

  // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array that we will update during the application lifetime.
  io.KeyMap [ImGuiKey_Tab]        = VK_TAB;
  io.KeyMap [ImGuiKey_LeftArrow]  = VK_LEFT;
  io.KeyMap [ImGuiKey_RightArrow] = VK_RIGHT;
  io.KeyMap [ImGuiKey_UpArrow]    = VK_UP;
  io.KeyMap [ImGuiKey_DownArrow]  = VK_DOWN;
  io.KeyMap [ImGuiKey_PageUp]     = VK_PRIOR;
  io.KeyMap [ImGuiKey_PageDown]   = VK_NEXT;
  io.KeyMap [ImGuiKey_Home]       = VK_HOME;
  io.KeyMap [ImGuiKey_End]        = VK_END;
  io.KeyMap [ImGuiKey_Delete]     = VK_DELETE;
  io.KeyMap [ImGuiKey_Backspace]  = VK_BACK;
  io.KeyMap [ImGuiKey_Enter]      = VK_RETURN;
  io.KeyMap [ImGuiKey_Escape]     = VK_ESCAPE;
  io.KeyMap [ImGuiKey_A]          = 'A';
  io.KeyMap [ImGuiKey_C]          = 'C';
  io.KeyMap [ImGuiKey_V]          = 'V';
  io.KeyMap [ImGuiKey_X]          = 'X';
  io.KeyMap [ImGuiKey_Y]          = 'Y';
  io.KeyMap [ImGuiKey_Z]          = 'Z';

  io.RenderDrawListsFn  = ImGui_ImplGL3_RenderDrawLists;       // Alternatively you can set this to NULL and call ImGui::GetDrawData() after ImGui::Render() to get the same ImDrawData pointer.
  //io.SetClipboardTextFn = ImGui_ImplGL3_SetClipboardText;
  //io.GetClipboardTextFn = ImGui_ImplGL3_GetClipboardText;
  io.ClipboardUserData  = game_window.hWnd;

  return true;
}

void
ImGui_ImplGL3_Shutdown (void)
{
  ImGui_ImplGL3_InvalidateDeviceObjects ();
  ImGui::Shutdown ();
}

void
SK_ImGui_PollGamepad (void);

void
ImGui_ImplGL3_NewFrame (void)
{
  if (! g_FontTexture)
    ImGui_ImplGlfwGL3_CreateDeviceObjects ();

  ImGuiIO& io (ImGui::GetIO ());

  RECT client;
  GetClientRect (game_window.hWnd, &client);

  // Setup display size (every frame to accommodate for window resizing)
  int w = client.right  - client.left,
      h = client.bottom - client.top;

  io.DisplaySize             =
    ImVec2 ( static_cast <float> (w), static_cast <float> (h) );
  io.DisplayFramebufferScale = 
    ImVec2 ( static_cast <float> (w), static_cast <float> (h) );
  //ImVec2 (w > 0 ? ((float)display_w / w) : 0, h > 0 ? ((float)display_h / h) : 0);

  // Setup time step
  INT64 current_time;

  QueryPerformanceCounter_Original (
    reinterpret_cast <LARGE_INTEGER *> (&current_time)
  );

  io.DeltaTime = static_cast <float> (current_time - g_Time) /
                 static_cast <float> (g_TicksPerSecond);
  g_Time       =                      current_time;

  // Read keyboard modifiers inputS
  io.KeyCtrl   = (io.KeysDown [VK_CONTROL]) != 0;
  io.KeyShift  = (io.KeysDown [VK_SHIFT])   != 0;
  io.KeyAlt    = (io.KeysDown [VK_MENU])    != 0;

  io.KeySuper  = false;


  // For games that hijack the mouse cursor using Direct Input 8.
  //
  //  -- Acquire actually means release their exclusive ownership : )
  //
  //if (SK_ImGui_WantMouseCapture ())
  //  SK_Input_DI8Mouse_Acquire ();
  //else
  //  SK_Input_DI8Mouse_Release ();


  SK_ImGui_PollGamepad ();

  // Start the frame
  ImGui::NewFrame ();
}