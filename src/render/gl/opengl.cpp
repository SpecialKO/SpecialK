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

#pragma warning ( disable : 4273 )

#define __SK_SUBSYSTEM__ L"  OpenGL  "

#define WIN32_LEAN_AND_MEAN

#include <cstdint>

#define _L2(w)  L ## w
#define  _L(w) _L2(w)

#define _GDI32_

#include <SpecialK/utility/bidirectional_map.h>

#include <SpecialK/window.h>
#include <SpecialK/import.h>
#include <SpecialK/hooks.h>
#include <SpecialK/injection/injection.h>

#include <SpecialK/osd/popup.h>

// For OpenGL-IK
#include <SpecialK/render/dxgi/dxgi_util.h>
#include <SpecialK/render/dxgi/dxgi_hdr.h>

#include <SpecialK/render/gl/opengl_backend.h>
#include <imgui/backends/imgui_gl3.h>
#include <../depends/include/GL/glew.h>
#include <../depends/include/GL/wglew.h>
#include <imgui/imgui.h>

//SK_OpenGL_KnownPrograms SK_GL_Programs;
//SK_OpenGL_KnownTextures SK_GL_Textures;
//SK_OpenGL_KnownBuffers  SK_GL_Buffers;

SK_Thread_HybridSpinlock *cs_gl_ctx = nullptr;
HGLRC          __gl_primary_context = nullptr;

SK_LazyGlobal <std::unordered_map <HGLRC, HGLRC>> __gl_shared_contexts;
SK_LazyGlobal <std::unordered_map <HGLRC, BOOL>>  init_;

struct SK_GL_Context {
};

// Count the number of contexts seen
//
//  * Identifies games that actually USE OpenGL, rather than
//      strangely call SetPixelFormat and nothing else...
int  SK_GL_ContextCount  = 0;
bool SK_GL_OnD3D11       = false;
bool SK_GL_OnD3D11_Reset = false;

unsigned int SK_GL_SwapHook = 0;
volatile LONG __gl_ready = FALSE;

void __stdcall
SK_GL_UpdateRenderStats (void);

using wglGetProcAddress_pfn = PROC (WINAPI *)(LPCSTR);
      wglGetProcAddress_pfn
      wgl_get_proc_address  = nullptr;

using wglSwapBuffers_pfn = BOOL (WINAPI *)(HDC);
      wglSwapBuffers_pfn
      wgl_swap_buffers   = nullptr;

// TODO: This stuff is tied to an actual ICD context, it should not be global
//
using wglSwapIntervalEXT_pfn = BOOL (WINAPI *)(int);
      wglSwapIntervalEXT_pfn
      wgl_swap_interval      = nullptr;

using wglGetSwapIntervalEXT_pfn = int (WINAPI *)(void);
      wglGetSwapIntervalEXT_pfn
      wgl_get_swap_interval     = nullptr;

using SwapBuffers_pfn  = BOOL (WINAPI *)(HDC);
      SwapBuffers_pfn
      gdi_swap_buffers = nullptr;

using wglSetPixelFormat_pfn = BOOL (WINAPI *)(HDC hDC, DWORD PixelFormat, CONST PIXELFORMATDESCRIPTOR *pdf);
      wglSetPixelFormat_pfn
      wgl_set_pixel_format = nullptr;

using wglCreateContextAttribs_pfn = HGLRC (WINAPI *)(HDC hDC, HGLRC hshareContext, const int *attribList);
      wglCreateContextAttribs_pfn
      wgl_create_context_attribs = nullptr;

using wglChoosePixelFormatARB_pfn = BOOL (WINAPI *)(HDC     hdc,
                                             const int   *piAttribIList,
                                             const FLOAT *pfAttribFList,
                                             UINT          nMaxFormats,
                                             int          *piFormats,
                                             UINT         *nNumFormats);
      wglChoosePixelFormatARB_pfn
      wgl_choose_pixel_format_arb = nullptr;

static ULONG GL_HOOKS  = 0UL;

__declspec (noinline)
int
WINAPI
wglChoosePixelFormat ( HDC                     hDC,
                 const PIXELFORMATDESCRIPTOR* ppfd );


__declspec (noinline)
BOOL
WINAPI
wglSetPixelFormat ( HDC                     hDC,
                    int                     iPixelFormat,
              const PIXELFORMATDESCRIPTOR* ppfd );


using glRenderbufferStorage_pfn  = void (WINAPI *)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
      glRenderbufferStorage_pfn  gl_renderbuffer_storage  = nullptr;

using glNamedRenderbufferStorage_pfn = void (WINAPI *)(GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height);
      glNamedRenderbufferStorage_pfn  gl_named_renderbuffer_storage  = nullptr;

__declspec (noinline) void WINAPI glRenderbufferStorage_SK      (GLenum target,       GLenum internalformat, GLsizei width, GLsizei height);
__declspec (noinline) void WINAPI glNamedRenderbufferStorage_SK (GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height);

using glFramebufferTexture_pfn      = void (WINAPI *)(GLenum target,      GLenum attachment,                   GLuint texture, GLint level);
using glFramebufferTexture2D_pfn    = void (WINAPI *)(GLenum target,      GLenum attachment, GLenum textarget, GLuint texture, GLint level);
using glNamedFramebufferTexture_pfn = void (WINAPI *)(GLuint framebuffer, GLenum attachment,                   GLuint texture, GLint level);
      glFramebufferTexture_pfn      gl_framebuffer_texture       = nullptr;
      glFramebufferTexture2D_pfn    gl_framebuffer_texture2d     = nullptr;
      glNamedFramebufferTexture_pfn gl_named_framebuffer_texture = nullptr;

__declspec (noinline) void WINAPI glNamedFramebufferTexture_SK (GLuint framebuffer, GLenum attachment,                   GLuint texture, GLint level);
__declspec (noinline) void WINAPI glFramebufferTexture_SK      (GLenum target,      GLenum attachment,                   GLuint texture, GLint level);
__declspec (noinline) void WINAPI glFramebufferTexture2D_SK    (GLenum target,      GLenum attachment, GLenum textarget, GLuint texture, GLint level);

void
WaitForInit_GL (void)
{
  return;
  //SK_Thread_SpinUntilFlagged (&__gl_ready);
}

#include <SpecialK/osd/text.h>

static
HMODULE local_gl = nullptr;

using finish_pfn = void (WINAPI *)(void);

HMODULE
SK_LoadRealGL (void)
{
  wchar_t    wszBackendDLL [MAX_PATH + 2] = { };
  wcsncpy_s (wszBackendDLL, MAX_PATH, SK_GetSystemDirectory (), _TRUNCATE);
  lstrcatW  (wszBackendDLL, L"\\");

  lstrcatW  (wszBackendDLL, L"OpenGL32.dll");


  if (local_gl == nullptr)
    local_gl = SK_Modules->LoadLibrary (wszBackendDLL);
  else {
    HMODULE hMod;
    GetModuleHandleExW (0x00, wszBackendDLL, &hMod);
  }

  return local_gl;
}

void
SK_FreeRealGL (void)
{
  SK_FreeLibrary (local_gl);
}


void
WINAPI
opengl_init_callback (finish_pfn finish)
{
  SK_BootOpenGL ();

  SK_ICommandProcessor *pCommandProc = nullptr;

  SK_RunOnce (
    pCommandProc =
      SK_Render_InitializeSharedCVars ()
  );

  if (pCommandProc != nullptr)
  {
    // TODO: Any OpenGL Specific Console Variables..?
  }

  finish ();
}

bool
SK::OpenGL::Startup (void)
{
  bool ret =
    SK_StartupCore (L"OpenGL32", static_cast_pfn <void*> (opengl_init_callback));

  return ret;
}

bool
SK::OpenGL::Shutdown (void)
{
  return SK_ShutdownCore (L"OpenGL32");
}


extern "C"
{
#define OPENGL_STUB(_Return, _Name, _Proto, _Args)                       \
    typedef _Return (WINAPI *imp_##_Name##_pfn) _Proto;                  \
    static imp_##_Name##_pfn imp_##_Name = nullptr;                      \
                                                                         \
  _Return WINAPI                                                         \
  _Name _Proto {                                                         \
    if (imp_##_Name == nullptr) {                                        \
                                                                         \
      SK_LoadRealGL ();                                                  \
                                                                         \
      const char * const szName = #_Name;                                \
      imp_##_Name=(imp_##_Name##_pfn)SK_GetProcAddress(local_gl, szName);\
                                                                         \
      if (imp_##_Name == nullptr) {                                      \
        dll_log->Log (                                                   \
          L"[ OpenGL32 ] Unable to locate symbol %s in OpenGL32.dll",    \
          L#_Name);                                                      \
        return 0;                                                        \
      }                                                                  \
    }                                                                    \
                                                                         \
    return imp_##_Name _Args;                                            \
}

#define OPENGL_STUB_(_Name, _Proto, _Args)                               \
    typedef void (WINAPI *imp_##_Name##_pfn) _Proto;                     \
    static imp_##_Name##_pfn imp_##_Name = nullptr;                      \
                                                                         \
  void WINAPI                                                            \
  _Name _Proto {                                                         \
    if (imp_##_Name == nullptr) {                                        \
                                                                         \
      SK_LoadRealGL ();                                                  \
                                                                         \
      const char * const szName = #_Name;                                \
      imp_##_Name=(imp_##_Name##_pfn)SK_GetProcAddress(local_gl, szName);\
                                                                         \
      if (imp_##_Name == nullptr) {                                      \
        dll_log->Log (                                                   \
          L"[ OpenGL32 ] Unable to locate symbol %s in OpenGL32.dll",    \
          L#_Name);                                                      \
        return;                                                          \
      }                                                                  \
    }                                                                    \
                                                                         \
    imp_##_Name _Args;                                                   \
}

#if 1
typedef uint32_t  GLenum;
typedef uint8_t   GLboolean;
typedef uint32_t  GLbitfield;
typedef int8_t    GLbyte;
typedef int16_t   GLshort;
typedef int32_t   GLint;
typedef int32_t   GLsizei; // ?
typedef uint8_t   GLubyte;
typedef uint16_t  GLushort;
typedef uint32_t  GLuint;
typedef float     GLfloat;
typedef float     GLclampf;
typedef double    GLdouble;
typedef double    GLclampd;
typedef void      GLvoid;
#else
using GLenum     = unsigned int;
using GLboolean  = unsigned char;
using GLbitfield = unsigned int;
using GLbyte     = signed   char;
using GLshort    = short;
using GLint      = int;
using GLsizei    = int;
using GLubyte    = unsigned char;
using GLushort   = unsigned short;
using GLuint     = unsigned int;
using GLfloat    = float;
using GLclampf   = float;
using GLdouble   = double;
using GLclampd   = double;
using GLvoid     = void;
#endif


OPENGL_STUB_(glAccum,    (GLenum op,GLfloat value),
                         (       op,        value));
OPENGL_STUB_(glAlphaFunc,(GLenum func,GLclampf ref),
                         (       func,         ref));

OPENGL_STUB(GLboolean,glAreTexturesResident,(GLsizei n,const GLuint *textures,GLboolean *residences),
                                            (        n,              textures,           residences));

OPENGL_STUB_(glArrayElement,(GLint i),
                            (      i));
OPENGL_STUB_(glBegin,       (GLenum mode),
                            (       mode));
OPENGL_STUB_(glBindTexture, (GLenum target,GLuint texture),
                            (       target,       texture));
OPENGL_STUB_(glBitmap,      (GLsizei width,GLsizei height,GLfloat xorig,GLfloat yorig,GLfloat xmove,GLfloat ymove,const GLubyte *bitmap),
                            (        width,        height,        xorig,        yorig,        xmove,        ymove,               bitmap));
OPENGL_STUB_(glBlendFunc,   (GLenum sfactor,GLenum dfactor),
                            (       sfactor,       dfactor));
OPENGL_STUB_(glCallList,    (GLuint list),
                            (       list));
OPENGL_STUB_(glCallLists,   (GLsizei n,GLenum type,const GLvoid *lists),
                            (        n,       type,              lists));
OPENGL_STUB_(glClear,       (GLbitfield mask),
                            (           mask));
OPENGL_STUB_(glClearAccum,  (GLfloat red,GLfloat green,GLfloat blue,GLfloat alpha),
                            (        red,        green,        blue,        alpha));
OPENGL_STUB_(glClearColor,  (GLclampf red,GLclampf green,GLclampf blue,GLclampf alpha),
                            (         red,         green,         blue,         alpha));
OPENGL_STUB_(glClearDepth,  (GLclampd depth),
                            (         depth));
OPENGL_STUB_(glClearIndex,  (GLfloat c),
                            (        c));
OPENGL_STUB_(glClearStencil,(GLint s),
                            (      s));
OPENGL_STUB_(glClipPlane,   (GLenum plane,const GLdouble *equation),
                            (       plane,                equation));

OPENGL_STUB_(glColor3b,  (GLbyte red,GLbyte green,GLbyte blue),
                         (       red,       green,       blue));
OPENGL_STUB_(glColor3bv, (const GLbyte *v),
                         (              v));
OPENGL_STUB_(glColor3d,  (GLdouble red,GLdouble green,GLdouble blue),
                         (         red,         green,         blue));
OPENGL_STUB_(glColor3dv, (const GLdouble *v),
                         (                v));
OPENGL_STUB_(glColor3f,  (GLfloat red,GLfloat green,GLfloat blue),
                         (        red,        green,        blue));
OPENGL_STUB_(glColor3fv, (const GLfloat *v),
                         (               v));
OPENGL_STUB_(glColor3i,  (GLint red,GLint green,GLint blue),
                         (      red,      green,      blue));
OPENGL_STUB_(glColor3iv, (const GLint *v),
                         (             v));
OPENGL_STUB_(glColor3s,  (GLshort red,GLshort green,GLshort blue),
                         (        red,        green,        blue));
OPENGL_STUB_(glColor3sv, (const GLshort *v),
                         (               v));
OPENGL_STUB_(glColor3ub, (GLubyte red,GLubyte green,GLubyte blue),
                         (        red,        green,        blue));
OPENGL_STUB_(glColor3ubv,(const GLubyte *v),
                         (               v));
OPENGL_STUB_(glColor3ui, (GLuint red,GLuint green,GLuint blue),
                         (       red,       green,       blue));
OPENGL_STUB_(glColor3uiv,(const GLuint *v),
                         (              v));
OPENGL_STUB_(glColor3us, (GLushort red,GLushort green,GLushort blue),
                         (         red,         green,         blue));
OPENGL_STUB_(glColor3usv,(const GLushort *v),
                         (                v));
OPENGL_STUB_(glColor4b,  (GLbyte red,GLbyte green,GLbyte blue,GLbyte alpha),
                         (       red,       green,       blue,       alpha));
OPENGL_STUB_(glColor4bv, (const GLbyte *v),
                         (              v));
OPENGL_STUB_(glColor4d,  (GLdouble red,GLdouble green,GLdouble blue,GLdouble alpha),
                         (         red,         green,         blue,         alpha));
OPENGL_STUB_(glColor4dv, (const GLdouble *v),
                         (                v));
OPENGL_STUB_(glColor4f,  (GLfloat red,GLfloat green,GLfloat blue,GLfloat alpha),
                         (        red,        green,        blue,        alpha));
OPENGL_STUB_(glColor4fv, (const GLfloat *v),
                         (               v));
OPENGL_STUB_(glColor4i,  (GLint red,GLint green,GLint blue,GLint alpha),
                         (      red,      green,      blue,      alpha));
OPENGL_STUB_(glColor4iv, (const GLint *v),
                         (             v));
OPENGL_STUB_(glColor4s,  (GLshort red,GLshort green,GLshort blue,GLshort alpha),
                         (        red,        green,        blue,        alpha));
OPENGL_STUB_(glColor4sv, (const GLshort *v),
                         (               v));
OPENGL_STUB_(glColor4ub, (GLubyte red,GLubyte green,GLubyte blue,GLubyte alpha),
                         (        red,        green,        blue,        alpha));
OPENGL_STUB_(glColor4ubv,(const GLubyte *v),
                         (               v));
OPENGL_STUB_(glColor4ui, (GLuint red,GLuint green,GLuint blue,GLuint alpha),
                         (       red,       green,       blue,       alpha));
OPENGL_STUB_(glColor4uiv,(const GLuint *v),
                         (              v));
OPENGL_STUB_(glColor4us, (GLushort red,GLushort green,GLushort blue,GLushort alpha),
                         (         red,         green,         blue,         alpha));
OPENGL_STUB_(glColor4usv,(const GLushort *v),
                         (                v));
OPENGL_STUB_(glColorMask,(GLboolean red,GLboolean green,GLboolean blue,GLboolean alpha),
                         (          red,          green,          blue,          alpha));

OPENGL_STUB_(glColorMaterial,(GLenum face,GLenum mode),
                             (       face,       mode));
OPENGL_STUB_(glColorPointer, (GLint size,GLenum type,GLsizei stride,const GLvoid *pointer),
                             (      size,       type,        stride,              pointer));
OPENGL_STUB_(glCopyPixels,   (GLint x,GLint y,GLsizei width,GLsizei height,GLenum type),
                             (      x,      y,        width,        height,       type));
OPENGL_STUB_(glCopyTexImage1D,(GLenum target,GLint level,GLenum internalFormat,GLint x,GLint y,GLsizei width,GLint border),
                              (       target,      level,       internalFormat,      x,      y,        width,      border));
OPENGL_STUB_(glCopyTexImage2D,(GLenum target,GLint level,GLenum internalFormat,GLint x,GLint y,GLsizei width,GLsizei height,GLint border),
                              (       target,      level,       internalFormat,      x,      y,        width,        height,      border));

OPENGL_STUB_(glCopyTexSubImage1D, (GLenum target,GLint level,GLint xoffset,GLint x,GLint y,GLsizei width),
                                  (       target,      level,      xoffset,      x,      y,        width));
OPENGL_STUB_(glCopyTexSubImage2D, (GLenum target,GLint level,GLint xoffset,GLint yoffset,GLint x,GLint y,GLsizei width,GLsizei height),
                                  (       target,      level,      xoffset,      yoffset,      x,      y,        width,        height));

OPENGL_STUB_(glCullFace,          (GLenum mode),
                                  (       mode));

// ???
OPENGL_STUB(GLint,glDebugEntry,   (GLint unknown0, GLint unknown1),
                                  (      unknown0,       unknown1));

OPENGL_STUB_(glDeleteLists,       (GLuint list,GLsizei range),
                                  (       list,        range));
OPENGL_STUB_(glDeleteTextures,    (GLsizei n,const GLuint *textures),
                                  (        n,              textures));

OPENGL_STUB_(glDepthFunc,         (GLenum func),
                                  (       func));
OPENGL_STUB_(glDepthMask,         (GLboolean flag),
                                  (          flag));
OPENGL_STUB_(glDepthRange,        (GLclampd zNear,GLclampd zFar),
                                  (         zNear,         zFar));
OPENGL_STUB_(glDisable,           (GLenum cap),
                                  (       cap));
OPENGL_STUB_(glDisableClientState,(GLenum array),
                                  (       array));

OPENGL_STUB_(glDrawArrays,        (GLenum mode,GLint first,GLsizei count),
                                  (       mode,      first,        count));
OPENGL_STUB_(glDrawBuffer,        (GLenum mode),
                                  (       mode));
OPENGL_STUB_(glDrawElements,      (GLenum mode,GLsizei count,GLenum type,const GLvoid *indices),
                                  (       mode,        count,       type,              indices));
OPENGL_STUB_(glDrawPixels,        (GLsizei width,GLsizei height,GLenum format,GLenum type,const GLvoid *pixels),
                                  (        width,        height,       format,       type,              pixels));

OPENGL_STUB_(glEdgeFlag,          (GLboolean flag),
                                  (          flag));
OPENGL_STUB_(glEdgeFlagPointer,   (GLsizei stride,const GLvoid *pointer),
                                  (        stride,              pointer));
OPENGL_STUB_(glEdgeFlagv,         (const GLboolean *flag),
                                  (                 flag));

OPENGL_STUB_(glEnable,            (GLenum cap),
                                  (       cap));
OPENGL_STUB_(glEnableClientState, (GLenum array),
                                  (       array));

OPENGL_STUB_(glEnd,               (void),());
OPENGL_STUB_(glEndList,           (void),());

OPENGL_STUB_(glEvalCoord1d, (GLdouble u),
                            (         u));
OPENGL_STUB_(glEvalCoord1dv,(const GLdouble *u),
                            (                u));
OPENGL_STUB_(glEvalCoord1f, (GLfloat u),
                            (        u));
OPENGL_STUB_(glEvalCoord1fv,(const GLfloat *u),
                            (               u));
OPENGL_STUB_(glEvalCoord2d, (GLdouble u,GLdouble v),
                            (         u,         v));
OPENGL_STUB_(glEvalCoord2dv,(const GLdouble *u),
                            (                u));
OPENGL_STUB_(glEvalCoord2f, (GLfloat u,GLfloat v),
                            (        u,        v));
OPENGL_STUB_(glEvalCoord2fv,(const GLfloat *u),
                            (               u));
OPENGL_STUB_(glEvalMesh1,   (GLenum mode,GLint i1,GLint i2),
                            (       mode,      i1,      i2));
OPENGL_STUB_(glEvalMesh2,   (GLenum mode,GLint i1,GLint i2,GLint j1,GLint j2),
                            (       mode,      i1,      i2,      j1,      j2));
OPENGL_STUB_(glEvalPoint1,  (GLint i),
                            (      i));
OPENGL_STUB_(glEvalPoint2,  (GLint i,GLint j),
                            (      i,      j));

OPENGL_STUB_(glFeedbackBuffer,(GLsizei size,GLenum type,GLfloat *buffer),
                              (        size,       type,         buffer));

OPENGL_STUB_(glFinish,(void),());
OPENGL_STUB_(glFlush, (void),());

OPENGL_STUB_(glFogf, (GLenum pname,GLfloat param),
                     (       pname,        param));
OPENGL_STUB_(glFogfv,(GLenum pname,const GLfloat *params),
                     (       pname,               params));
OPENGL_STUB_(glFogi, (GLenum pname,GLint param),
                     (       pname,      param));
OPENGL_STUB_(glFogiv,(GLenum pname,const GLint *params),
                     (       pname,             params));

OPENGL_STUB_(glFrontFace, (GLenum mode),
                          (       mode));
OPENGL_STUB_(glFrustum,   (GLdouble left,GLdouble right,GLdouble bottom,GLdouble top,GLdouble zNear,GLdouble zFar),
                          (         left,         right,         bottom,         top,         zNear,         zFar));

OPENGL_STUB(GLuint,glGenLists,(GLsizei range),
                              (        range));
OPENGL_STUB_(glGenTextures,   (GLsizei n,GLuint *textures),
                              (        n,        textures));

OPENGL_STUB_(glGetBooleanv, (GLenum pname,GLboolean *params),
                            (       pname,           params));
OPENGL_STUB_(glGetClipPlane,(GLenum plane,GLdouble *equation),
                            (       plane,          equation));
OPENGL_STUB_(glGetDoublev,  (GLenum pname,GLdouble *params),
                            (       pname,          params));

OPENGL_STUB(GLenum,glGetError,(void),());

OPENGL_STUB_(glGetFloatv,        (GLenum pname,GLfloat *params),
                                 (       pname,         params));
OPENGL_STUB_(glGetIntegerv,      (GLenum pname,GLint *params),
                                 (       pname,       params));
OPENGL_STUB_(glGetLightfv,       (GLenum light,GLenum pname,GLfloat *params),
                                 (       light,       pname,         params));
OPENGL_STUB_(glGetLightiv,       (GLenum light,GLenum pname,GLint *params),
                                 (       light,       pname,       params));
OPENGL_STUB_(glGetMapdv,         (GLenum target,GLenum query,GLdouble *v),
                                 (       target,       query,          v));
OPENGL_STUB_(glGetMapfv,         (GLenum target,GLenum query,GLfloat *v),
                                 (       target,       query,         v));
OPENGL_STUB_(glGetMapiv,         (GLenum target,GLenum query,GLint *v),
                                 (       target,       query,       v));

OPENGL_STUB_(glGetMaterialfv,    (GLenum face, GLenum pname, GLfloat *params),
                                 (       face,        pname,          params));
OPENGL_STUB_(glGetMaterialiv,    (GLenum face, GLenum pname, GLint *params),
                                 (       face,        pname,        params));
OPENGL_STUB_(glGetPixelMapfv,    (GLenum map, GLfloat *values),
                                 (       map,          values));
OPENGL_STUB_(glGetPixelMapuiv,   (GLenum map, GLuint *values),
                                 (       map,         values));
OPENGL_STUB_(glGetPixelMapusv,   (GLenum map, GLushort *values),
                                 (       map,           values));
OPENGL_STUB_(glGetPointerv,      (GLenum pname, GLvoid* *params),
                                 (       pname,          params));
OPENGL_STUB_(glGetPolygonStipple,(GLubyte *mask),
                                 (         mask));

OPENGL_STUB(const GLubyte *,glGetString,(GLenum name),
                                        (       name));

OPENGL_STUB_(glGetTexEnvfv,            (GLenum target, GLenum pname, GLfloat *params),
                                       (       target,        pname,          params));
OPENGL_STUB_(glGetTexEnviv,            (GLenum target, GLenum pname, GLint *params),
                                       (       target,        pname,        params));
OPENGL_STUB_(glGetTexGendv,            (GLenum coord, GLenum pname, GLdouble *params),
                                       (       coord,        pname,           params));
OPENGL_STUB_(glGetTexGenfv,            (GLenum coord, GLenum pname, GLfloat *params),
                                       (       coord,        pname,          params));
OPENGL_STUB_(glGetTexGeniv,            (GLenum coord, GLenum pname, GLint *params),
                                       (       coord,        pname,        params));
OPENGL_STUB_(glGetTexImage,            (GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels),
                                       (       target,       level,        format,        type,         pixels));
OPENGL_STUB_(glGetTexLevelParameterfv, (GLenum target, GLint level, GLenum pname, GLfloat *params),
                                       (       target,       level,        pname,          params));
OPENGL_STUB_(glGetTexLevelParameteriv, (GLenum target, GLint level, GLenum pname, GLint *params),
                                       (       target,       level,        pname,        params));
OPENGL_STUB_(glGetTexParameterfv,      (GLenum target, GLenum pname, GLfloat *params),
                                       (       target,        pname,          params));
OPENGL_STUB_(glGetTexParameteriv,      (GLenum target, GLenum pname, GLint *params),
                                       (       target,        pname,        params));

OPENGL_STUB_(glHint,(GLenum target, GLenum mode),
                    (       target,        mode));

OPENGL_STUB_(glIndexMask,    (GLuint mask),
                             (       mask));
OPENGL_STUB_(glIndexPointer, (GLenum type, GLsizei stride, const GLvoid *pointer),
                             (       type,         stride,               pointer));
OPENGL_STUB_(glIndexd,       (GLdouble c),
                             (         c));
OPENGL_STUB_(glIndexdv,      (const GLdouble *c),
                             (                c));
OPENGL_STUB_(glIndexf,       (GLfloat c),
                             (        c));
OPENGL_STUB_(glIndexfv,      (const GLfloat *c),
                             (               c));
OPENGL_STUB_(glIndexi,       (GLint c),
                             (      c));
OPENGL_STUB_(glIndexiv,      (const GLint *c),
                             (             c));
OPENGL_STUB_(glIndexs,       (GLshort c),
                             (        c));
OPENGL_STUB_(glIndexsv,      (const GLshort *c),
                             (               c));
OPENGL_STUB_(glIndexub,      (GLubyte c),
                             (        c));
OPENGL_STUB_(glIndexubv,     (const GLubyte *c),
                             (               c));

OPENGL_STUB_(glInitNames,(void),());

OPENGL_STUB_(glInterleavedArrays,(GLenum format, GLsizei stride, const GLvoid *pointer),
                                 (       format,         stride,               pointer));

OPENGL_STUB(GLboolean,glIsEnabled,(GLenum cap),
                                  (       cap));
OPENGL_STUB(GLboolean,glIsList,   (GLuint list),
                                  (       list));
OPENGL_STUB(GLboolean,glIsTexture,(GLuint texture),
                                  (       texture));

OPENGL_STUB_(glLightModelf,  (GLenum pname, GLfloat param),
                             (       pname,         param));
OPENGL_STUB_(glLightModelfv, (GLenum pname, const GLfloat *params),
                             (       pname,                params));
OPENGL_STUB_(glLightModeli,  (GLenum pname, GLint param),
                             (       pname,       param));
OPENGL_STUB_(glLightModeliv, (GLenum pname, const GLint *params),
                             (       pname,              params));
OPENGL_STUB_(glLightf,       (GLenum light, GLenum pname, GLfloat param),
                             (       light,        pname,         param));
OPENGL_STUB_(glLightfv,      (GLenum light, GLenum pname, const GLfloat *params),
                             (       light,        pname,                params));
OPENGL_STUB_(glLighti,       (GLenum light, GLenum pname, GLint param),
                             (       light,        pname,       param));
OPENGL_STUB_(glLightiv,      (GLenum light, GLenum pname, const GLint *params),
                             (       light,        pname,              params));

OPENGL_STUB_(glLineStipple, (GLint factor, GLushort pattern),
                            (      factor,          pattern));
OPENGL_STUB_(glLineWidth,   (GLfloat width),
                            (        width));

OPENGL_STUB_(glListBase, (GLuint base),
                         (       base));

OPENGL_STUB_(glLoadIdentity,(void),());
OPENGL_STUB_(glLoadMatrixd, (const GLdouble *m),
                            (                m));
OPENGL_STUB_(glLoadMatrixf, (const GLfloat *m),
                            (               m));
OPENGL_STUB_(glLoadName,    (GLuint name),
                            (       name));

OPENGL_STUB_(glLogicOp, (GLenum opcode),
                        (       opcode));

OPENGL_STUB_(glMap1d,    (GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points),
                         (       target,          u1,          u2,       stride,       order,                 points));
OPENGL_STUB_(glMap1f,    (GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points),
                         (       target,         u1,         u2,       stride,       order,                points));
OPENGL_STUB_(glMap2d,    (GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points),
                         (       target,          u1,          u2,       ustride,       uorder,          v1,          v2,       vstride,       vorder,                 points));
OPENGL_STUB_(glMap2f,    (GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points),
                         (       target,         u1,         u2,       ustride,       uorder,         v1,         v2,       vstride,       vorder,                points));
OPENGL_STUB_(glMapGrid1d,(GLint un, GLdouble u1, GLdouble u2),
                         (      un,          u1,          u2));
OPENGL_STUB_(glMapGrid1f,(GLint un, GLfloat u1, GLfloat u2),
                         (      un,         u1,         u2));
OPENGL_STUB_(glMapGrid2d,(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2),
                         (      un,          u1,          u2,       vn,          v1,          v2));
OPENGL_STUB_(glMapGrid2f,(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2),
                         (      un,         u1,         u2,       vn,         v1,         v2));

OPENGL_STUB_(glMaterialf,  (GLenum face, GLenum pname, GLfloat param),
                           (       face,        pname,         param));
OPENGL_STUB_(glMaterialfv, (GLenum face, GLenum pname, const GLfloat *params),
                           (       face,        pname,                params));
OPENGL_STUB_(glMateriali,  (GLenum face, GLenum pname, GLint param),
                           (       face,        pname,       param));
OPENGL_STUB_(glMaterialiv, (GLenum face, GLenum pname, const GLint *params),
                           (       face,        pname,              params));

OPENGL_STUB_(glMatrixMode, (GLenum mode),
                           (       mode));
OPENGL_STUB_(glMultMatrixd,(const GLdouble *m),
                           (               m));
OPENGL_STUB_(glMultMatrixf,(const GLfloat *m),
                           (               m));

OPENGL_STUB_(glNewList, (GLuint list, GLenum mode),
                        (       list,        mode));

OPENGL_STUB_(glNormal3b,      (GLbyte nx, GLbyte ny, GLbyte nz),
                              (       nx,        ny,        nz));
OPENGL_STUB_(glNormal3bv,     (const GLbyte *v),
                              (              v));
OPENGL_STUB_(glNormal3d,      (GLdouble nx, GLdouble ny, GLdouble nz),
                              (         nx,          ny,          nz));
OPENGL_STUB_(glNormal3dv,     (const GLdouble *v),
                              (                v));
OPENGL_STUB_(glNormal3f,      (GLfloat nx, GLfloat ny, GLfloat nz),
                              (        nx,         ny,         nz));
OPENGL_STUB_(glNormal3fv,     (const GLfloat *v),
                              (               v));
OPENGL_STUB_(glNormal3i,      (GLint nx, GLint ny, GLint nz),
                              (      nx,       ny,       nz));
OPENGL_STUB_(glNormal3iv,     (const GLint *v),
                              (             v));
OPENGL_STUB_(glNormal3s,      (GLshort nx, GLshort ny, GLshort nz),
                              (        nx,         ny,         nz));
OPENGL_STUB_(glNormal3sv,     (const GLshort *v),
                              (               v));
OPENGL_STUB_(glNormalPointer, (GLenum type, GLsizei stride, const GLvoid *pointer),
                              (       type,         stride,               pointer));

OPENGL_STUB_(glOrtho, (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar),
                      (         left,          right,          bottom,          top,          zNear,          zFar));

OPENGL_STUB_(glPassThrough, (GLfloat token),
                            (        token));

OPENGL_STUB_(glPixelMapfv,     (GLenum map, GLsizei mapsize, const GLfloat *values),
                               (       map,         mapsize,                values));
OPENGL_STUB_(glPixelMapuiv,    (GLenum map, GLsizei mapsize, const GLuint *values),
                               (       map,         mapsize,               values));
OPENGL_STUB_(glPixelMapusv,    (GLenum map, GLsizei mapsize, const GLushort *values),
                               (       map,         mapsize,                 values));
OPENGL_STUB_(glPixelStoref,    (GLenum pname, GLfloat param),
                               (       pname,         param));
OPENGL_STUB_(glPixelStorei,    (GLenum pname, GLint param),
                               (       pname,       param));
OPENGL_STUB_(glPixelTransferf, (GLenum pname, GLfloat param),
                               (       pname,         param));
OPENGL_STUB_(glPixelTransferi, (GLenum pname, GLint param),
                               (       pname,       param));
OPENGL_STUB_(glPixelZoom,      (GLfloat xfactor, GLfloat yfactor),
                               (        xfactor,         yfactor));

OPENGL_STUB_(glPointSize,      (GLfloat size),
                               (        size));
OPENGL_STUB_(glPolygonMode,    (GLenum face, GLenum mode),
                               (       face,        mode));
OPENGL_STUB_(glPolygonOffset,  (GLfloat factor, GLfloat units),
                               (        factor,         units));
OPENGL_STUB_(glPolygonStipple, (const GLubyte *mask),
                               (               mask));

OPENGL_STUB_(glPopAttrib,      (void),());
OPENGL_STUB_(glPopClientAttrib,(void),());
OPENGL_STUB_(glPopMatrix,      (void),());
OPENGL_STUB_(glPopName,        (void),());

OPENGL_STUB_(glPrioritizeTextures, (GLsizei n, const GLuint *textures, const GLclampf *priorities),
                                   (        n,               textures,                 priorities));

OPENGL_STUB_(glPushAttrib,      (GLbitfield mask),
                                (           mask));
OPENGL_STUB_(glPushClientAttrib,(GLbitfield mask),
                                (           mask));
OPENGL_STUB_(glPushMatrix,      (void),());
OPENGL_STUB_(glPushName,        (GLuint name),
                                (       name));

OPENGL_STUB_(glRasterPos2d,  (GLdouble x, GLdouble y),
                             (         x,          y));
OPENGL_STUB_(glRasterPos2dv, (const GLdouble *v),
                             (                v));
OPENGL_STUB_(glRasterPos2f,  (GLfloat x, GLfloat y),
                             (        x,         y));
OPENGL_STUB_(glRasterPos2fv, (const GLfloat *v),
                             (               v));
OPENGL_STUB_(glRasterPos2i,  (GLint x, GLint y),
                             (      x,       y));
OPENGL_STUB_(glRasterPos2iv, (const GLint *v),
                             (             v));
OPENGL_STUB_(glRasterPos2s,  (GLshort x, GLshort y),
                             (        x,         y));
OPENGL_STUB_(glRasterPos2sv, (const GLshort *v),
                             (               v));
OPENGL_STUB_(glRasterPos3d,  (GLdouble x, GLdouble y, GLdouble z),
                             (         x,          y,          z));
OPENGL_STUB_(glRasterPos3dv, (const GLdouble *v),
                             (                v));
OPENGL_STUB_(glRasterPos3f,  (GLfloat x, GLfloat y, GLfloat z),
                             (        x,         y,         z));
OPENGL_STUB_(glRasterPos3fv, (const GLfloat *v),
                             (               v));
OPENGL_STUB_(glRasterPos3i,  (GLint x, GLint y, GLint z),
                             (      x,       y,       z));
OPENGL_STUB_(glRasterPos3iv, (const GLint *v),
                             (             v));
OPENGL_STUB_(glRasterPos3s,  (GLshort x, GLshort y, GLshort z),
                             (        x,         y,         z));
OPENGL_STUB_(glRasterPos3sv, (const GLshort *v),
                             (               v));
OPENGL_STUB_(glRasterPos4d,  (GLdouble x, GLdouble y, GLdouble z, GLdouble w),
                             (         x,          y,          z,          w));
OPENGL_STUB_(glRasterPos4dv, (const GLdouble *v),
                             (                v));
OPENGL_STUB_(glRasterPos4f,  (GLfloat x, GLfloat y, GLfloat z, GLfloat w),
                             (        x,         y,         z,         w));
OPENGL_STUB_(glRasterPos4fv, (const GLfloat *v),
                             (               v));
OPENGL_STUB_(glRasterPos4i,  (GLint x, GLint y, GLint z, GLint w),
                             (      x,       y,       z,       w));
OPENGL_STUB_(glRasterPos4iv, (const GLint *v),
                             (             v));
OPENGL_STUB_(glRasterPos4s,  (GLshort x, GLshort y, GLshort z, GLshort w),
                             (        x,         y,         z,         w));
OPENGL_STUB_(glRasterPos4sv, (const GLshort *v),
                             (               v));

OPENGL_STUB_(glReadBuffer, (GLenum mode),
                           (       mode));
OPENGL_STUB_(glReadPixels, (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels),
                           (      x,       y,         width,         height,        format,        type,         pixels));

OPENGL_STUB_(glRectd,  (GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2),
                       (         x1,          y1,          x2,          y2));
OPENGL_STUB_(glRectdv, (const GLdouble *v1, const GLdouble *v2),
                       (                v1,                 v2));
OPENGL_STUB_(glRectf,  (GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2),
                       (        x1,         y1,         x2,         y2));
OPENGL_STUB_(glRectfv, (const GLfloat *v1, const GLfloat *v2),
                       (               v1,                v2));
OPENGL_STUB_(glRecti,  (GLint x1, GLint y1, GLint x2, GLint y2),
                       (      x1,       y1,       x2,       y2));
OPENGL_STUB_(glRectiv, (const GLint *v1, const GLint *v2),
                       (             v1,              v2));
OPENGL_STUB_(glRects,  (GLshort x1, GLshort y1, GLshort x2, GLshort y2),
                       (        x1,         y1,         x2,         y2));
OPENGL_STUB_(glRectsv, (const GLshort *v1, const GLshort *v2),
                       (               v1,                v2));

OPENGL_STUB(GLint, glRenderMode, (GLenum mode),
                                 (       mode));

OPENGL_STUB_(glRotated, (GLdouble angle, GLdouble x, GLdouble y, GLdouble z),
                        (         angle,          x,          y,          z));
OPENGL_STUB_(glRotatef, (GLfloat angle, GLfloat x, GLfloat y, GLfloat z),
                        (        angle,         x,         y,         z));

OPENGL_STUB_(glScaled, (GLdouble x, GLdouble y, GLdouble z),
                       (         x,          y,          z));
OPENGL_STUB_(glScalef, (GLfloat x, GLfloat y, GLfloat z),
                       (        x,         y,         z));

OPENGL_STUB_(glScissor, (GLint x, GLint y, GLsizei width, GLsizei height),
                        (      x,       y,         width,         height));

OPENGL_STUB_(glSelectBuffer, (GLsizei size, GLuint *buffer),
                             (        size,         buffer));

OPENGL_STUB_(glShadeModel, (GLenum mode),
                           (       mode));

OPENGL_STUB_(glStencilFunc, (GLenum func, GLint ref, GLuint mask),
                            (       func,       ref,        mask));
OPENGL_STUB_(glStencilMask, (GLuint mask),
                            (       mask));
OPENGL_STUB_(glStencilOp,   (GLenum fail, GLenum zfail, GLenum zpass),
                            (       fail,        zfail,        zpass));

OPENGL_STUB_(glTexCoord1d,      (GLdouble s),
                                (         s));
OPENGL_STUB_(glTexCoord1dv,     (const GLdouble *v),
                                (                v));
OPENGL_STUB_(glTexCoord1f,      (GLfloat s),
                                (        s));
OPENGL_STUB_(glTexCoord1fv,     (const GLfloat *v),
                                (               v));
OPENGL_STUB_(glTexCoord1i,      (GLint s),
                                (      s));
OPENGL_STUB_(glTexCoord1iv,     (const GLint *v),
                                (             v));
OPENGL_STUB_(glTexCoord1s,      (GLshort s),
                                (        s));
OPENGL_STUB_(glTexCoord1sv,     (const GLshort *v),
                                (               v));
OPENGL_STUB_(glTexCoord2d,      (GLdouble s, GLdouble t),
                                (         s,          t));
OPENGL_STUB_(glTexCoord2dv,     (const GLdouble *v),
                                (                v));
OPENGL_STUB_(glTexCoord2f,      (GLfloat s, GLfloat t),
                                (        s,         t));
OPENGL_STUB_(glTexCoord2fv,     (const GLfloat *v),
                                (               v));
OPENGL_STUB_(glTexCoord2i,      (GLint s, GLint t),
                                (      s,       t));
OPENGL_STUB_(glTexCoord2iv,     (const GLint *v),
                                (             v));
OPENGL_STUB_(glTexCoord2s,      (GLshort s, GLshort t),
                                (        s,         t));
OPENGL_STUB_(glTexCoord2sv,     (const GLshort *v),
                                (               v));
OPENGL_STUB_(glTexCoord3d,      (GLdouble s, GLdouble t, GLdouble r),
                                (         s,          t,          r));
OPENGL_STUB_(glTexCoord3dv,     (const GLdouble *v),
                                (                v));
OPENGL_STUB_(glTexCoord3f,      (GLfloat s, GLfloat t, GLfloat r),
                                (        s,         t,         r));
OPENGL_STUB_(glTexCoord3fv,     (const GLfloat *v),
                                (               v));
OPENGL_STUB_(glTexCoord3i,      (GLint s, GLint t, GLint r),
                                (      s,       t,       r));
OPENGL_STUB_(glTexCoord3iv,     (const GLint *v),
                                (             v));
OPENGL_STUB_(glTexCoord3s,      (GLshort s, GLshort t, GLshort r),
                                (        s,         t,         r));
OPENGL_STUB_(glTexCoord3sv,     (const GLshort *v),
                                (               v));
OPENGL_STUB_(glTexCoord4d,      (GLdouble s, GLdouble t, GLdouble r, GLdouble q),
                                (         s,          t,          r,          q));
OPENGL_STUB_(glTexCoord4dv,     (const GLdouble *v),
                                (                v));
OPENGL_STUB_(glTexCoord4f,      (GLfloat s, GLfloat t, GLfloat r, GLfloat q),
                                (        s,         t,         r,         q));
OPENGL_STUB_(glTexCoord4fv,     (const GLfloat *v),
                                (               v));
OPENGL_STUB_(glTexCoord4i,      (GLint s, GLint t, GLint r, GLint q),
                                (      s,       t,       r,       q));
OPENGL_STUB_(glTexCoord4iv,     (const GLint *v),
                                (             v));
OPENGL_STUB_(glTexCoord4s,      (GLshort s, GLshort t, GLshort r, GLshort q),
                                (        s,         t,         r,         q));
OPENGL_STUB_(glTexCoord4sv,     (const GLshort *v),
                                (               v));
OPENGL_STUB_(glTexCoordPointer, (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer),
                                (      size,        type,         stride,               pointer));

OPENGL_STUB_(glTexEnvf,  (GLenum target, GLenum pname, GLfloat param),
                         (       target,        pname,         param));
OPENGL_STUB_(glTexEnvfv, (GLenum target, GLenum pname, const GLfloat *params),
                         (       target,        pname,                params));
OPENGL_STUB_(glTexEnvi,  (GLenum target, GLenum pname, GLint param),
                         (       target,        pname,       param));
OPENGL_STUB_(glTexEnviv, (GLenum target, GLenum pname, const GLint *params),
                         (       target,        pname,              params));
OPENGL_STUB_(glTexGend,  (GLenum coord, GLenum pname, GLdouble param),
                         (       coord,        pname,          param));
OPENGL_STUB_(glTexGendv, (GLenum coord, GLenum pname, const GLdouble *params),
                         (       coord,        pname,                 params));
OPENGL_STUB_(glTexGenf,  (GLenum coord, GLenum pname, GLfloat param),
                         (       coord,        pname,         param));
OPENGL_STUB_(glTexGenfv, (GLenum coord, GLenum pname, const GLfloat *params),
                         (       coord,        pname,                params));
OPENGL_STUB_(glTexGeni,  (GLenum coord, GLenum pname, GLint param),
                         (       coord,        pname,       param));
OPENGL_STUB_(glTexGeniv, (GLenum coord, GLenum pname, const GLint *params),
                         (       coord,        pname,              params));

OPENGL_STUB_(glTexImage1D,     (GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels),
                               (       target,       level,       internalformat,         width,       border,        format,        type,               pixels));
OPENGL_STUB_(glTexImage2D,     (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels),
                               (       target,       level,       internalformat,         width,         height,       border,        format,        type,               pixels));
OPENGL_STUB_(glTexParameterf,  (GLenum target, GLenum pname, GLfloat param),
                               (       target,        pname,         param));
OPENGL_STUB_(glTexParameterfv, (GLenum target, GLenum pname, const GLfloat *params),
                               (       target,        pname,                params));
OPENGL_STUB_(glTexParameteri,  (GLenum target, GLenum pname, GLint param),
                               (       target,        pname,       param));
OPENGL_STUB_(glTexParameteriv, (GLenum target, GLenum pname, const GLint *params),
                               (       target,        pname,              params));
OPENGL_STUB_(glTexSubImage1D,  (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels),
                               (       target,       level,       xoffset,         width,        format,        type,               pixels));
OPENGL_STUB_(glTexSubImage2D,  (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels),
                               (       target,       level,       xoffset,       yoffset,         width,         height,        format,        type,               pixels));

OPENGL_STUB_(glTranslated, (GLdouble x, GLdouble y, GLdouble z),
                           (         x,          y,          z));
OPENGL_STUB_(glTranslatef, (GLfloat x, GLfloat y, GLfloat z),
                           (        x,         y,         z));

OPENGL_STUB_(glVertex2d,  (GLdouble x, GLdouble y),
                          (         x,          y));
OPENGL_STUB_(glVertex2dv, (const GLdouble *v),
                          (                v));
OPENGL_STUB_(glVertex2f,  (GLfloat x, GLfloat y),
                          (        x,         y));
OPENGL_STUB_(glVertex2fv, (const GLfloat *v),
                          (               v));
OPENGL_STUB_(glVertex2i,  (GLint x, GLint y),
                          (      x,       y));
OPENGL_STUB_(glVertex2iv, (const GLint *v),
                          (             v));
OPENGL_STUB_(glVertex2s,  (GLshort x, GLshort y),
                          (        x,         y));
OPENGL_STUB_(glVertex2sv, (const GLshort *v),
                          (               v));
OPENGL_STUB_(glVertex3d,  (GLdouble x, GLdouble y, GLdouble z),
                          (         x,          y,          z));
OPENGL_STUB_(glVertex3dv, (const GLdouble *v),
                          (                v));
OPENGL_STUB_(glVertex3f,  (GLfloat x, GLfloat y, GLfloat z),
                          (        x,         y,         z));
OPENGL_STUB_(glVertex3fv, (const GLfloat *v),
                          (               v));
OPENGL_STUB_(glVertex3i,  (GLint x, GLint y, GLint z),
                          (      x,       y,       z));
OPENGL_STUB_(glVertex3iv, (const GLint *v),
                          (             v));
OPENGL_STUB_(glVertex3s,  (GLshort x, GLshort y, GLshort z),
                          (        x,         y,         z));
OPENGL_STUB_(glVertex3sv, (const GLshort *v),
                          (               v));
OPENGL_STUB_(glVertex4d,  (GLdouble x, GLdouble y, GLdouble z, GLdouble w),
                          (         x,          y ,         z,          w));
OPENGL_STUB_(glVertex4dv, (const GLdouble *v),
                          (                v));
OPENGL_STUB_(glVertex4f,  (GLfloat x, GLfloat y, GLfloat z, GLfloat w),
                          (        x,         y,         z,         w));
OPENGL_STUB_(glVertex4fv, (const GLfloat *v),
                          (               v));
OPENGL_STUB_(glVertex4i,  (GLint x, GLint y, GLint z, GLint w),
                          (      x,       y,       z,       w));
OPENGL_STUB_(glVertex4iv, (const GLint *v),
                          (             v));
OPENGL_STUB_(glVertex4s,  (GLshort x, GLshort y, GLshort z, GLshort w),
                          (        x,         y,         z,         w));
OPENGL_STUB_(glVertex4sv, (const GLshort *v),
                          (               v));

OPENGL_STUB_(glVertexPointer, (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer),
                              (      size,        type,         stride,               pointer));
OPENGL_STUB_(glViewport,      (GLint x, GLint y, GLsizei width, GLsizei height),
                              (      x,       y,         width,         height));

#if 0
/* EXT_vertex_array */
typedef void (APIENTRY * PFNGLARRAYELEMENTEXTPROC) (GLint i);
typedef void (APIENTRY * PFNGLDRAWARRAYSEXTPROC) (GLenum mode, GLint first, GLsizei count);
typedef void (APIENTRY * PFNGLVERTEXPOINTEREXTPROC) (GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
typedef void (APIENTRY * PFNGLNORMALPOINTEREXTPROC) (GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
typedef void (APIENTRY * PFNGLCOLORPOINTEREXTPROC) (GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
typedef void (APIENTRY * PFNGLINDEXPOINTEREXTPROC) (GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
typedef void (APIENTRY * PFNGLTEXCOORDPOINTEREXTPROC) (GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
typedef void (APIENTRY * PFNGLEDGEFLAGPOINTEREXTPROC) (GLsizei stride, GLsizei count, const GLboolean *pointer);
typedef void (APIENTRY * PFNGLGETPOINTERVEXTPROC) (GLenum pname, GLvoid* *params);
typedef void (APIENTRY * PFNGLARRAYELEMENTARRAYEXTPROC)(GLenum mode, GLsizei count, const GLvoid* pi);

/* WIN_draw_range_elements */
typedef void (APIENTRY * PFNGLDRAWRANGEELEMENTSWINPROC) (GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices);

/* WIN_swap_hint */
typedef void (APIENTRY * PFNGLADDSWAPHINTRECTWINPROC)  (GLint x, GLint y, GLsizei width, GLsizei height);

/* EXT_paletted_texture */
typedef void (APIENTRY * PFNGLCOLORTABLEEXTPROC)
(GLenum target, GLenum internalFormat, GLsizei width, GLenum format,
  GLenum type, const GLvoid *data);
typedef void (APIENTRY * PFNGLCOLORSUBTABLEEXTPROC)
(GLenum target, GLsizei start, GLsizei count, GLenum format,
  GLenum type, const GLvoid *data);
typedef void (APIENTRY * PFNGLGETCOLORTABLEEXTPROC)
(GLenum target, GLenum format, GLenum type, GLvoid *data);
typedef void (APIENTRY * PFNGLGETCOLORTABLEPARAMETERIVEXTPROC)
(GLenum target, GLenum pname, GLint *params);
typedef void (APIENTRY * PFNGLGETCOLORTABLEPARAMETERFVEXTPROC)
(GLenum target, GLenum pname, GLfloat *params);
#endif

OPENGL_STUB(BOOL, wglCopyContext, (HGLRC h1, HGLRC h2, UINT u),
                                  (      h1,       h2,      u));
//OPENGL_STUB(HGLRC,wglCreateContext, (HDC hDC),
//                                    (    hDC));


OPENGL_STUB(HGLRC,wglCreateLayerContext, (HDC hDC, int idx),
                                         (    hDC,     idx));
OPENGL_STUB(HGLRC,wglGetCurrentContext,  (VOID), ());
OPENGL_STUB(HDC,  wglGetCurrentDC,       (VOID), ());
OPENGL_STUB(PROC, wglGetProcAddress,     (LPCSTR str),
                                         (       str));
//OPENGL_STUB(BOOL, wglShareLists,         (HGLRC hglrc1, HGLRC hglrc2),
//                                         (      hglrc1,       hglrc2));
OPENGL_STUB(BOOL, wglUseFontBitmapsA, (HDC hDC, DWORD dw0, DWORD dw1, DWORD dw2),
                                      (    hDC,       dw0,       dw1,       dw2));
OPENGL_STUB(BOOL, wglUseFontBitmapsW, (HDC hDC, DWORD dw0, DWORD dw1, DWORD dw2),
                                      (    hDC,       dw0,       dw1,       dw2));

//OPENGL_STUB(INT, wglChoosePixelFormat, (HDC hDC, CONST PIXELFORMATDESCRIPTOR *pfd),
//                                       (    hDC,                              pfd));

OPENGL_STUB(BOOL, wglGetPixelFormat, (HDC hDC),
                                     (    hDC));

/* Layer plane descriptor */
//typedef struct tagLAYERPLANEDESCRIPTOR { // lpd
//  WORD  nSize;
//  WORD  nVersion;
//  DWORD dwFlags;
//  BYTE  iPixelType;
//  BYTE  cColorBits;
//  BYTE  cRedBits;
//  BYTE  cRedShift;
//  BYTE  cGreenBits;
//  BYTE  cGreenShift;
//  BYTE  cBlueBits;
//  BYTE  cBlueShift;
//  BYTE  cAlphaBits;
//  BYTE  cAlphaShift;
//  BYTE  cAccumBits;
//  BYTE  cAccumRedBits;
//  BYTE  cAccumGreenBits;
//  BYTE  cAccumBlueBits;
//  BYTE  cAccumAlphaBits;
//  BYTE  cDepthBits;
//  BYTE  cStencilBits;
//  BYTE  cAuxBuffers;
//  BYTE  iLayerPlane;
//  BYTE  bReserved;
//  COLORREF crTransparent;
//} LAYERPLANEDESCRIPTOR, *PLAYERPLANEDESCRIPTOR, FAR *LPLAYERPLANEDESCRIPTOR;

//OPENGL_STUB(BOOL, wglDescribeLayerPlane, (HDC hDC, DWORD PixelFormat, DWORD LayerPlane, UINT nBytes, LPLAYERPLANEDESCRIPTOR lpd),
//                                         (    hDC,       PixelFormat,       LayerPlane,      nBytes,                        lpd));

OPENGL_STUB(DWORD, wglDescribePixelFormat, (HDC hDC, DWORD PixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR pfd),
                                           (    hDC,       PixelFormat,      nBytes,                         pfd));

//OPENGL_STUB(DWORD, wglGetLayerPaletteEntries, (HDC hDC, DWORD LayerPlane, DWORD Start, DWORD Entries, COLORREF *cr),
//                                              (    hDC,       LayerPlane,       Start,       Entries,           cr));

//OPENGL_STUB(BOOL, wglRealizeLayerPalette, (HDC hDC, DWORD LayerPlane, BOOL Realize),
//                                          (    hDC,       LayerPlane,      Realize));

//OPENGL_STUB(DWORD, wglSetLayerPaletteEntries, (HDC hDC, DWORD LayerPlane, DWORD Start, DWORD Entries, CONST COLORREF *cr),
//                                              (    hDC,       LayerPlane,       Start,       Entries,                 cr));

//OPENGL_STUB(BOOL, wglSetPixelFormat, (HDC hDC, DWORD PixelFormat, CONST PIXELFORMATDESCRIPTOR *pdf),
//                                     (    hDC,       PixelFormat,                              pdf));


OPENGL_STUB(BOOL, wglSwapLayerBuffers, ( HDC hDC, UINT nPlanes ),
                                       (     hDC,      nPlanes ));


//typedef struct _POINTFLOAT {
//  FLOAT   x;
//  FLOAT   y;
//} POINTFLOAT, *PPOINTFLOAT;
//
//typedef struct _GLYPHMETRICSFLOAT {
//  FLOAT       gmfBlackBoxX;
//  FLOAT       gmfBlackBoxY;
//  POINTFLOAT  gmfptGlyphOrigin;
//  FLOAT       gmfCellIncX;
//  FLOAT       gmfCellIncY;
//} GLYPHMETRICSFLOAT, *PGLYPHMETRICSFLOAT, FAR *LPGLYPHMETRICSFLOAT;

OPENGL_STUB(BOOL,wglUseFontOutlinesA,(HDC hDC, DWORD dw0, DWORD dw1, DWORD dw2, FLOAT f0, FLOAT f1, int i0, LPGLYPHMETRICSFLOAT pgmf),
                                     (    hDC,       dw0,       dw1,       dw2,       f0,       f1,     i0,                     pgmf));
OPENGL_STUB(BOOL,wglUseFontOutlinesW,(HDC hDC, DWORD dw0, DWORD dw1, DWORD dw2, FLOAT f0, FLOAT f1, int i0, LPGLYPHMETRICSFLOAT pgmf),
                                     (    hDC,       dw0,       dw1,       dw2,       f0,       f1,     i0,                     pgmf));
}



void SK_GL_ClipControl::push (GLint origin, GLint depth_mode)
{
  if (glClipControl == nullptr && wgl_get_proc_address != nullptr)
      glClipControl =       (glClipControl_pfn)
      wgl_get_proc_address ("glClipControl");

  if (glClipControl == nullptr)
    return;

  glGetIntegerv (GL_CLIP_ORIGIN,     &original.origin);
  glGetIntegerv (GL_CLIP_DEPTH_MODE, &original.depth_mode);

  glClipControl (origin, depth_mode);
}

void SK_GL_ClipControl::pop (void)
{
  if (glClipControl == nullptr)
    return;

  glClipControl (original.origin, original.depth_mode);
}

SK_GL_ClipControl::glClipControl_pfn
SK_GL_ClipControl::glClipControl = nullptr;

#define SK_GL_GhettoStateBlock_Capture()                                                                              \
  GLint     last_program;              glGetIntegerv   (GL_CURRENT_PROGRAM,              &last_program);              \
  GLint     last_texture;              glGetIntegerv   (GL_TEXTURE_BINDING_2D,           &last_texture);              \
  GLint     last_active_texture;       glGetIntegerv   (GL_ACTIVE_TEXTURE,               &last_active_texture);       \
                                       glActiveTexture (GL_TEXTURE0);                                                 \
  GLint     last_texture0;             glGetIntegerv   (GL_TEXTURE_BINDING_2D,           &last_texture0);             \
  GLint     last_sampler0;             glGetIntegerv   (GL_SAMPLER_BINDING,              &last_sampler0);             \
  GLint     last_drawbuffer;           glGetIntegerv   (GL_DRAW_FRAMEBUFFER_BINDING,     &last_drawbuffer);           \
  GLint     last_readbuffer;           glGetIntegerv   (GL_READ_FRAMEBUFFER_BINDING,     &last_readbuffer);           \
  GLint     last_framebuffer;          glGetIntegerv   (GL_FRAMEBUFFER_BINDING,          &last_framebuffer);          \
  GLint     last_array_buffer;         glGetIntegerv   (GL_ARRAY_BUFFER_BINDING,         &last_array_buffer);         \
  GLint     last_element_array_buffer; glGetIntegerv   (GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer); \
  GLint     last_vertex_array;         glGetIntegerv   (GL_VERTEX_ARRAY_BINDING,         &last_vertex_array);         \
  GLint     last_blend_src;            glGetIntegerv   (GL_BLEND_SRC,                    &last_blend_src);            \
  GLint     last_blend_dst;            glGetIntegerv   (GL_BLEND_DST,                    &last_blend_dst);            \
  GLint     last_blend_equation_rgb;   glGetIntegerv   (GL_BLEND_EQUATION_RGB,           &last_blend_equation_rgb);   \
  GLint     last_blend_equation_alpha; glGetIntegerv   (GL_BLEND_EQUATION_ALPHA,         &last_blend_equation_alpha); \
  GLint     last_viewport    [4];      glGetIntegerv   (GL_VIEWPORT,                      last_viewport);             \
  GLint     last_scissor_box [4];      glGetIntegerv   (GL_SCISSOR_BOX,                   last_scissor_box);          \
  GLboolean last_color_mask  [4];      glGetBooleanv   (GL_COLOR_WRITEMASK,               last_color_mask);           \
  GLboolean last_enable_blend        = glIsEnabled     (GL_BLEND);                                                    \
  GLboolean last_enable_cull_face    = glIsEnabled     (GL_CULL_FACE);                                                \
  GLboolean last_enable_depth_test   = glIsEnabled     (GL_DEPTH_TEST);                                               \
  GLboolean last_enable_scissor_test = glIsEnabled     (GL_SCISSOR_TEST);                                             \
  GLboolean last_enable_stencil_test = glIsEnabled     (GL_STENCIL_TEST);                                             \
  GLboolean last_srgb_framebuffer    = glIsEnabled     (GL_FRAMEBUFFER_SRGB);                                         \
                                                                                                                      \
  SK_GL_ClipControl clip_control;                                                                                     \
                    clip_control.push (GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);


#define SK_GL_GhettoStateBlock_Apply()                                                                  \
  glUseProgram            (                         last_program);                                      \
  glBindVertexArray       (                         last_vertex_array);                                 \
  glBindFramebuffer       (GL_FRAMEBUFFER,          last_framebuffer);                                  \
                                                                                                        \
  if (last_readbuffer != last_framebuffer)                                                              \
    glBindFramebuffer     (GL_READ_BUFFER,          last_readbuffer);                                   \
  if (last_drawbuffer != last_framebuffer)                                                              \
    glBindFramebuffer     (GL_DRAW_BUFFER,          last_drawbuffer);                                   \
                                                                                                        \
  glActiveTexture         (GL_TEXTURE0);                                                                \
  glBindTexture           (GL_TEXTURE_2D,           last_texture0);                                     \
  glBindSampler           (0,                       last_sampler0);                                     \
                                                                                                        \
  glActiveTexture         (last_active_texture);                                                        \
  glBindTexture           (GL_TEXTURE_2D,           last_texture);                                      \
                                                                                                        \
  /* TODO: Shader Pipeline Objects (the above objects fully capture all cocos2d state encapsulation) */ \
                                                                                                        \
  glBlendEquationSeparate (last_blend_equation_rgb, last_blend_equation_alpha);                         \
  glBlendFunc             (last_blend_src,          last_blend_dst);                                    \
                                                                                                        \
  if (last_srgb_framebuffer)    glEnable (GL_FRAMEBUFFER_SRGB); else glDisable (GL_FRAMEBUFFER_SRGB);   \
  if (last_enable_stencil_test) glEnable (GL_STENCIL_TEST);     else glDisable (GL_STENCIL_TEST);       \
  if (last_enable_blend)        glEnable (GL_BLEND);            else glDisable (GL_BLEND);              \
  if (last_enable_cull_face)    glEnable (GL_CULL_FACE);        else glDisable (GL_CULL_FACE);          \
  if (last_enable_depth_test)   glEnable (GL_DEPTH_TEST);       else glDisable (GL_DEPTH_TEST);         \
  if (last_enable_scissor_test) glEnable (GL_SCISSOR_TEST);     else glDisable (GL_SCISSOR_TEST);       \
                                                                                                        \
  glColorMask( last_color_mask  [0], last_color_mask  [1], last_color_mask  [2], last_color_mask  [3]); \
  glViewport ( last_viewport    [0], last_viewport    [1], last_viewport    [2], last_viewport    [3]); \
  glScissor  ( last_scissor_box [0], last_scissor_box [1], last_scissor_box [2], last_scissor_box [3]); \
                                                                                                        \
  clip_control.pop ();


static GLuint
     ceGL_VAO = 0;


typedef BOOL  (WINAPI *wglMakeCurrent_pfn)(HDC hDC, HGLRC hglrc);
                       wglMakeCurrent_pfn
                       wgl_make_current = nullptr;

typedef BOOL  (WINAPI *wglShareLists_pfn)(HGLRC ctx0, HGLRC ctx1);
                       wglShareLists_pfn
                       wgl_share_lists = nullptr;

typedef HGLRC (WINAPI *wglCreateContext_pfn)(HDC hDC);
                       wglCreateContext_pfn
                       wgl_create_context = nullptr;

typedef int   (WINAPI *wglChoosePixelFormat_pfn)(HDC hDC, const PIXELFORMATDESCRIPTOR *ppfd);
                       wglChoosePixelFormat_pfn
                       wgl_choose_pixel_format = nullptr;

typedef BOOL  (WINAPI *wglDeleteContext_pfn)(HGLRC hglrc);
                       wglDeleteContext_pfn
                       wgl_delete_context = nullptr;


__declspec (noinline)
BOOL
WINAPI
wglMakeCurrent (HDC hDC, HGLRC hglrc)
{
  WaitForInit_GL ();

  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  SK_LOG1 ( ( L"[%x (tid=%04x)]  wglMakeCurrent "
              L"(hDC=%x, hglrc=%x)",
                WindowFromDC         (hDC),
              SK_Thread_GetCurrentId (   ),
                                      hDC, hglrc ),
            L" OpenGL32 " );

  BOOL ret =
    wgl_make_current (hDC, hglrc);


  pTLS->render->gl->current_hglrc = hglrc;
  pTLS->render->gl->current_hdc   = hDC;

  return ret;
}

__declspec (noinline)
BOOL
WINAPI
wglDeleteContext (HGLRC hglrc)
{
  WaitForInit_GL ();

  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  if (config.system.log_level >= 0 && pTLS != nullptr)
  {
    SK_LOG0 ( ( L"[%08x (tid=%04x)]  wglDeleteContext "
                   L"(hglrc=%x)\t[%ws]",
                     WindowFromDC             (pTLS->render->gl->current_hdc),
                       SK_Thread_GetCurrentId (   ),
                                              hglrc, SK_GetCallerName ().c_str () ),
                L" OpenGL32 " );
  }


  if (pTLS == nullptr)
    return wgl_delete_context (hglrc);


  bool has_children = false;
  {
    std::scoped_lock <SK_Thread_HybridSpinlock> auto_lock (*cs_gl_ctx);

    for ( auto& it : __gl_shared_contexts.get () )
    {
      if (it.first == hglrc)
      {
        has_children = true;
      }

      else if (it.second == hglrc)
      {        it.second  = nullptr; }
    }

    if (has_children)
    {
      __gl_shared_contexts->erase (hglrc);
    }
  }


  if (__gl_primary_context == hglrc/* && (! has_children)*/)
  {
    if (! SK_GL_OnD3D11)
    {
      ImGui_ImplGL3_InvalidateDeviceObjects ();
    }

    init_.get () [__gl_primary_context] = false;
                  __gl_primary_context  = nullptr;
  }

  BOOL ret =
    wgl_delete_context (hglrc);

  if (ret)
  {
    if (hglrc == SK_GL_GetCurrentContext ())
    {
      wglMakeCurrent (SK_GL_GetCurrentDC (), nullptr);
    }
  }

  return ret;
}



#if 0
typedef uint32_t  GLenum;
typedef uint8_t   GLboolean;
typedef uint32_t  GLbitfield;
typedef int8_t    GLbyte;
typedef int16_t   GLshort;
typedef int32_t   GLint;
typedef int32_t   GLsizei; // ?
typedef uint8_t   GLubyte;
typedef uint16_t  GLushort;
typedef uint32_t  GLuint;
typedef float     GLfloat;
typedef float     GLclampf;
typedef double    GLdouble;
typedef double    GLclampd;
typedef void      GLvoid;
#else
typedef unsigned int   GLenum;
typedef unsigned char  GLboolean;
typedef unsigned int   GLbitfield;
typedef signed   char  GLbyte;
typedef short          GLshort;
typedef int            GLint;
typedef int            GLsizei;
typedef unsigned char  GLubyte;
typedef unsigned short GLushort;
typedef unsigned int   GLuint;
typedef float          GLfloat;
typedef float          GLclampf;
typedef double         GLdouble;
typedef double         GLclampd;
typedef void           GLvoid;
#endif

struct sk_gl_stateblock_s {
  GLint     _vao;
  GLint     _ibo;;
  GLint     _vbo;
  GLint     _ubo;
  GLint     _program;
  GLint     _textures2d [80],
            _samplers   [80];
  GLint     _active_texture;
  GLint     _viewport    [4];
  GLint     _scissor_box [4];
  GLint     _scissor_test;
  GLint     _blend;
  GLint     _blend_src,      _blend_dest;
  GLint     _blend_eq_color, _blend_eq_alpha;
  GLint     _depth_test;
  GLboolean _depth_mask;
  GLint     _depth_func;
  GLint     _stencil_test;
  GLint     _stencil_ref;
  GLint     _stencil_func;
  GLint     _stencil_op_fail,   _stencil_op_zfail, _stencil_op_zpass;
  GLint     _stencil_read_mask, _stencil_mask;
  GLint     _polygon_mode,      _frontface;
  GLint     _cullface,          _cullface_mode;
  GLint     _fbo;
  GLint     _read_fbo, _draw_fbo;
  GLint     _srgb;
  GLboolean _color_mask  [4];
  GLenum    _drawbuffers [8];

  GLuint    _temp_vao;
};

#if 0
#include <stack>
static __declspec (thread) std::stack <sk_gl_stateblock_s> __gl_state_stack;

void
SK_GL_PushMostStates (void)
{
  sk_gl_stateblock_s sb;

  glGetIntegerv (GL_VERTEX_ARRAY_BINDING,         &sb._vao);
  glGetIntegerv (GL_ARRAY_BUFFER_BINDING,         &sb._vbo);
  glGetIntegerv (GL_ELEMENT_ARRAY_BUFFER_BINDING, &sb._ibo);
  glGetIntegerv (GL_UNIFORM_BUFFER_BINDING,       &sb._ubo);
  glGetIntegerv (GL_CURRENT_PROGRAM,              &sb._program);
  glGetIntegerv (GL_ACTIVE_TEXTURE,               &sb._active_texture);

  for (GLuint i = 0; i < 80; i++)
  {
    glActiveTexture (GL_TEXTURE0 + i);
    glGetIntegerv (GL_TEXTURE_BINDING_2D, &sb._textures2d [i]);
    glGetIntegerv (GL_SAMPLER_BINDING,    &sb._samplers   [i]);
  }

  glGetIntegerv (GL_VIEWPORT,    sb._viewport);
  glGetIntegerv (GL_SCISSOR_BOX, sb._scissor_box);

  sb._scissor_test = glIsEnabled (GL_SCISSOR_TEST);
  sb._blend        = glIsEnabled (GL_BLEND);

  glGetIntegerv (GL_BLEND_SRC,            &sb._blend_src);
  glGetIntegerv (GL_BLEND_DST,            &sb._blend_dest);
  glGetIntegerv (GL_BLEND_EQUATION_RGB,   &sb._blend_eq_color);
  glGetIntegerv (GL_BLEND_EQUATION_ALPHA, &sb._blend_eq_alpha);

  sb._depth_test = glIsEnabled (GL_DEPTH_TEST);

  glGetBooleanv (GL_DEPTH_WRITEMASK, &sb._depth_mask);
  glGetIntegerv (GL_DEPTH_FUNC,      &sb._depth_func);

  sb._stencil_test = glIsEnabled (GL_STENCIL_TEST);

  glGetIntegerv (GL_STENCIL_REF,             &sb._stencil_ref);
  glGetIntegerv (GL_STENCIL_FUNC,            &sb._stencil_func);
  glGetIntegerv (GL_STENCIL_FAIL,            &sb._stencil_op_fail);
  glGetIntegerv (GL_STENCIL_PASS_DEPTH_FAIL, &sb._stencil_op_zfail);
  glGetIntegerv (GL_STENCIL_PASS_DEPTH_PASS, &sb._stencil_op_zpass);
  glGetIntegerv (GL_STENCIL_VALUE_MASK,      &sb._stencil_read_mask);
  glGetIntegerv (GL_STENCIL_WRITEMASK,       &sb._stencil_mask);
  glGetIntegerv (GL_POLYGON_MODE,            &sb._polygon_mode);
  glGetIntegerv (GL_FRONT_FACE,              &sb._frontface);

  sb._cullface = glIsEnabled (GL_CULL_FACE);

  glGetIntegerv (GL_CULL_FACE_MODE,           &sb._cullface_mode);
  glGetIntegerv (GL_FRAMEBUFFER_BINDING,      &sb._fbo);
  glGetIntegerv (GL_DRAW_FRAMEBUFFER_BINDING, &sb._draw_fbo);
  glGetIntegerv (GL_READ_FRAMEBUFFER_BINDING, &sb._read_fbo);

  sb._srgb = glIsEnabled (GL_FRAMEBUFFER_SRGB);

  glGetBooleanv (GL_COLOR_WRITEMASK, sb._color_mask);

  for (GLuint i = 0; i < 8; i++)
  {
    GLint                                       drawbuffer = GL_NONE;
    glGetIntegerv (GL_DRAW_BUFFER0 + i,        &drawbuffer);
    sb._drawbuffers [i] = static_cast <GLenum> (drawbuffer);
  }

  glGenVertexArrays (1, &sb._temp_vao);
  glBindVertexArray (sb._temp_vao);

  __gl_state_stack.push (sb);
}

void
SK_GL_PopMostStates (void)
{
  sk_gl_stateblock_s sb = __gl_state_stack.top ();
                          __gl_state_stack.pop ();

  glBindVertexArray (                         sb._vao);
  glBindBuffer      (GL_ELEMENT_ARRAY_BUFFER, sb._ibo);
  glBindBuffer      (GL_ARRAY_BUFFER,         sb._vbo);
  glBindBuffer      (GL_UNIFORM_BUFFER,       sb._ubo);
  glUseProgram      (                     sb._program);

  glBindFramebuffer (GL_FRAMEBUFFER, sb._fbo);

  if (sb._read_fbo != sb._fbo)
    glBindFramebuffer (GL_READ_BUFFER, sb._read_fbo);
  if (sb._draw_fbo != sb._fbo)
    glBindFramebuffer (GL_DRAW_BUFFER, sb._draw_fbo);

  for (GLuint i = 0; i < 80; i++)
  {
    glActiveTexture (GL_TEXTURE0 + i);
    glBindTexture   (GL_TEXTURE_2D, sb._textures2d [i]);
    glBindSampler   (i, sb._samplers [i]);
  }

  glActiveTexture ( sb._active_texture);
  glViewport      ( sb._viewport    [0], sb._viewport [1],
                    sb._viewport    [2], sb._viewport [3] );
  glScissor       ( sb._scissor_box [0], sb._scissor_box [1],
                    sb._scissor_box [2], sb._scissor_box [3] );

  if (sb._scissor_test) { glEnable  (GL_SCISSOR_TEST); }
  else                  { glDisable (GL_SCISSOR_TEST); }
  if (sb._blend)        { glEnable  (GL_BLEND);        }
  else                  { glDisable (GL_BLEND);        }

  glBlendFunc             (sb._blend_src,      sb._blend_dest);
  glBlendEquationSeparate (sb._blend_eq_color, sb._blend_eq_alpha);

  if (sb._depth_test) { glEnable  (GL_DEPTH_TEST); }
  else                { glDisable (GL_DEPTH_TEST); }

  glDepthMask (sb._depth_mask);
  glDepthFunc (sb._depth_func);

  if (sb._stencil_test) { glEnable  (GL_STENCIL_TEST); }
  else                  { glDisable (GL_STENCIL_TEST); }

  glStencilFunc (sb._stencil_func,    sb._stencil_ref,      sb._stencil_read_mask);
  glStencilOp   (sb._stencil_op_fail, sb._stencil_op_zfail, sb._stencil_op_zpass);
  glStencilMask (sb._stencil_mask);

  glPolygonMode (GL_FRONT_AND_BACK, sb._polygon_mode);
  glFrontFace   (                   sb._frontface);

  if (sb._cullface) { glEnable  (GL_CULL_FACE); }
  else              { glDisable (GL_CULL_FACE); }

  glCullFace (sb._cullface_mode);

  if (sb._srgb) { glEnable  (GL_FRAMEBUFFER_SRGB); }
  else          { glDisable (GL_FRAMEBUFFER_SRGB); }

  glColorMask (sb._color_mask [0],
               sb._color_mask [1],
               sb._color_mask [2],
               sb._color_mask [3] );

  if (sb._drawbuffers [1] == GL_NONE &&
      sb._drawbuffers [2] == GL_NONE &&
      sb._drawbuffers [3] == GL_NONE &&
      sb._drawbuffers [4] == GL_NONE &&
      sb._drawbuffers [5] == GL_NONE &&
      sb._drawbuffers [6] == GL_NONE &&
      sb._drawbuffers [7] == GL_NONE)
  {
    glDrawBuffer (sb._drawbuffers [0]);
  }
  else
  {
    glDrawBuffers (8, sb._drawbuffers);
  }

  glDeleteVertexArrays (1, &sb._temp_vao);
}
#endif



void
SK_Overlay_DrawGL (void)
{
  glPushAttrib      (GL_ALL_ATTRIB_BITS);

  SK_GL_GhettoStateBlock_Capture ();

  static RECT rect     = { -1, -1, -1, -1 };
         RECT rect_now = {  0,  0,  0,  0 };

  GetClientRect (SK_TLS_Bottom ()->render->gl->current_hwnd, &rect_now);

  rect = rect_now;


  //// Do not touch the default VAO state (assuming the context even has one)
  if (ceGL_VAO == 0 || (! glIsVertexArray (ceGL_VAO))) glGenVertexArrays (1, &ceGL_VAO);

  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  if (last_srgb_framebuffer)
    rb.framebuffer_flags |= SK_FRAMEBUFFER_FLAG_SRGB;
  else
    rb.framebuffer_flags &= SK_FRAMEBUFFER_FLAG_SRGB;

//glBindVertexArray (ceGL_VAO);
  glBindFramebuffer (GL_FRAMEBUFFER, 0);
  glDisable         (GL_FRAMEBUFFER_SRGB);
  glActiveTexture   (GL_TEXTURE0);
  glBindSampler     (0, 0);
  glViewport        (0, 0,
                  sk::narrow_cast <GLsizei> (rect.right  - rect.left),
                  sk::narrow_cast <GLsizei> (rect.bottom - rect.top));
  glScissor         (0, 0,
                  sk::narrow_cast <GLsizei> (rect.right  - rect.left),
                  sk::narrow_cast <GLsizei> (rect.bottom - rect.top));
  glColorMask       (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glDisable         (GL_STENCIL_TEST);
  glDisable         (GL_DEPTH_TEST);
  glDisable         (GL_CULL_FACE);
  glEnable          (GL_BLEND);


  // Queue-up Pre-SK OSD Screenshots
  SK_Screenshot_ProcessQueue (SK_ScreenshotStage::BeforeOSD, rb);

  SK_ImGui_DrawFrame (0x00, nullptr);

  SK_GL_GhettoStateBlock_Apply ();

  glPopAttrib ();


  // Queue-up Post-SK OSD Screenshots (Does not include ReShade)
  SK_Screenshot_ProcessQueue (SK_ScreenshotStage::PrePresent, rb);
}


#include <SpecialK/render/d3d11/d3d11_core.h>
#include <shaders/vs_gl_dx_interop.h>
#include <shaders/ps_gl_dx_interop.h>

static auto constexpr _DXBackBuffers = 3;

struct SK_IndirectX_InteropCtx;
struct SK_IndirectX_PresentManager {
  HANDLE          hThread         = INVALID_HANDLE_VALUE;
  HANDLE          hNotifyPresent  = INVALID_HANDLE_VALUE;
  HANDLE          hAckPresent     = INVALID_HANDLE_VALUE;
  HANDLE          hNotifyReset    = INVALID_HANDLE_VALUE;
  HANDLE          hAckReset       = INVALID_HANDLE_VALUE;
  volatile LONG64 frames          = 0;
  volatile LONG64 frames_complete = 0;

  int             interval        = 0;

  void Start   (SK_IndirectX_InteropCtx *pCtx);
  void Present (SK_IndirectX_InteropCtx *pCtx, int interval);
  void Reset   (SK_IndirectX_InteropCtx *pCtx);
};

struct SK_IndirectX_InteropCtx
{
  struct gl_s {
    GLuint                                 texture    = 0;
    GLuint                                 color_rbo  = 0;
    GLuint                                 fbo        = 0;
    HDC                                    hdc        = nullptr;
    HGLRC                                  hglrc      = nullptr;

    bool                                   fullscreen = false;
    UINT                                   width      = 0;
    UINT                                   height     = 0;
  } gl;

  struct d3d11_s {
    SK_ComPtr <IDXGIFactory2>              pFactory;
    SK_ComPtr <ID3D11Device>               pDevice;
    SK_ComPtr <ID3D11DeviceContext>        pDevCtx;
    HANDLE                                 hInteropDevice = nullptr;
    D3D_FEATURE_LEVEL                      featureLevel;

    struct staging_s {
      SK_ComPtr <ID3D11SamplerState>       colorSampler;
      SK_ComPtr <ID3D11Texture2D>          colorBuffer;
      SK_ComPtr <ID3D11ShaderResourceView> colorView;
      HANDLE                               hColorBuffer = nullptr;
    } staging; // Copy the finished image from another API here

    struct flipper_s {
      SK_ComPtr <ID3D11InputLayout>        pVertexLayout;
      SK_ComPtr <ID3D11VertexShader>       pVertexShader;
      SK_ComPtr <ID3D11PixelShader>        pPixelShader;
    } flipper; // GL's pixel origin is upside down, we must flip it

    SK_ComPtr <ID3D11BlendState>           pBlendState;
  } d3d11;

  struct output_s {
    SK_ComPtr <IDXGISwapChain1>            pSwapChain;
    HANDLE                                 hSemaphore = nullptr;

    struct backbuffer_s {
      SK_ComPtr <ID3D11Texture2D>          image;
      SK_ComPtr <ID3D11RenderTargetView>   rtv;
    } backbuffer;

    struct caps_s {
      BOOL                                 tearing      = FALSE;
      BOOL                                 flip_discard = FALSE;
    } caps;

    UINT                                   swapchain_flags = 0x0;
    DXGI_SWAP_EFFECT                       swap_effect     = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

    HWND                                   hWnd     = nullptr;
    HMONITOR                               hMonitor = nullptr;
    RECT                                   lastKnownRect = { };
    D3D11_VIEWPORT                         viewport      = { };
  } output;

  SK_IndirectX_PresentManager              present_man;

  bool     stale = false;
};


void
SK_IndirectX_PresentManager::Start (SK_IndirectX_InteropCtx *pCtx)
{
  static volatile LONG lLock = 0;

  if (! InterlockedCompareExchange (&lLock, 1, 0))
  {
    if (hNotifyPresent == INVALID_HANDLE_VALUE) hNotifyPresent = SK_CreateEvent (nullptr, FALSE, FALSE, nullptr);
    if (hAckPresent    == INVALID_HANDLE_VALUE) hAckPresent    = SK_CreateEvent (nullptr, FALSE, FALSE, nullptr);
    if (hNotifyReset   == INVALID_HANDLE_VALUE) hNotifyReset   = SK_CreateEvent (nullptr, FALSE, FALSE, nullptr);
    if (hAckReset      == INVALID_HANDLE_VALUE) hAckReset      = SK_CreateEvent (nullptr, FALSE, FALSE, nullptr);
    if (hThread        == INVALID_HANDLE_VALUE)
        hThread =
      SK_Thread_CreateEx ([](LPVOID pUser) ->
      DWORD
      {
        SK_Thread_SetCurrentPriority (THREAD_PRIORITY_TIME_CRITICAL);

        SK_IndirectX_InteropCtx* pCtx =
          (SK_IndirectX_InteropCtx *)pUser;

        HANDLE events [3] =
        {
                    __SK_DLL_TeardownEvent,
          pCtx->present_man.hNotifyPresent,
          pCtx->present_man.hNotifyReset
        };

        enum PresentEvents {
          Shutdown = WAIT_OBJECT_0,
          Present  = WAIT_OBJECT_0 + 1,
          Reset    = WAIT_OBJECT_0 + 2
        };

        do
        {
          DWORD dwWaitState =
            WaitForMultipleObjects ( _countof (events),
                                               events, FALSE, INFINITE );

          if (dwWaitState == PresentEvents::Reset)
          {
            if (pCtx->output.hSemaphore != nullptr)
            {
              WaitForSingleObject (pCtx->output.hSemaphore, 500UL);
            }

            auto& pDevCtx = pCtx->d3d11.pDevCtx.p;

            pDevCtx->RSSetViewports       (   1, &pCtx->output.viewport          );
            pDevCtx->PSSetShaderResources (0, 1, &pCtx->d3d11.staging.colorView.p);

            pDevCtx->IASetPrimitiveTopology (D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
            pDevCtx->IASetVertexBuffers     (0, 1, std::array <ID3D11Buffer *, 1> { nullptr }.data (),
                                                   std::array <UINT,           1> { 0       }.data (),
                                                   std::array <UINT,           1> { 0       }.data ());
            pDevCtx->IASetInputLayout       (pCtx->d3d11.flipper.pVertexLayout);
            pDevCtx->IASetIndexBuffer       (nullptr, DXGI_FORMAT_UNKNOWN, 0);

            static constexpr FLOAT fBlendFactor [4] = { 0.0f, 0.0f, 0.0f, 0.0f };

            pDevCtx->OMSetBlendState        (pCtx->d3d11.pBlendState,
                                                                 fBlendFactor, 0xFFFFFFFF  );
            pDevCtx->VSSetShader            (pCtx->d3d11.flipper.pVertexShader, nullptr, 0);
            pDevCtx->PSSetShader            (pCtx->d3d11.flipper.pPixelShader,  nullptr, 0);
            pDevCtx->PSSetSamplers   (0, 1, &pCtx->d3d11.staging.colorSampler.p);

            pDevCtx->RSSetState               (nullptr   );
            pDevCtx->RSSetScissorRects        (0, nullptr);
            pDevCtx->OMSetDepthStencilState   (nullptr, 0);

            if (pCtx->d3d11.pDevice->GetFeatureLevel () >= D3D_FEATURE_LEVEL_11_0)
            {
              pDevCtx->HSSetShader   (nullptr, nullptr, 0);
              pDevCtx->DSSetShader   (nullptr, nullptr, 0);
            }
            pDevCtx->GSSetShader     (nullptr, nullptr, 0);
            pDevCtx->SOSetTargets    (0, nullptr, nullptr);

            SK_GetCurrentRenderBackend ().setDevice (pCtx->d3d11.pDevice);

            SetEvent (pCtx->present_man.hAckReset);

            InterlockedCompareExchange (&lLock, 0, 1);
          }


          if (dwWaitState == PresentEvents::Present)
          {
            if (pCtx->output.hSemaphore != nullptr)
            {
              WaitForSingleObject (pCtx->output.hSemaphore, 100UL);
            }


            auto& pDevCtx = pCtx->d3d11.pDevCtx.p;



            InterlockedIncrement64 (&pCtx->present_man.frames);


            if (pCtx->d3d11.staging.colorView.p != nullptr)
            {
              pDevCtx->OMSetRenderTargets ( 1,
                        &pCtx->output.backbuffer.rtv.p, nullptr );
              pDevCtx->Draw  (              4,                0 );
              pDevCtx->OMSetRenderTargets ( 0, nullptr, nullptr );
            }

            pDevCtx->Flush ();


            auto pSwapChain =
              pCtx->output.pSwapChain.p;

            BOOL bSuccess =
              SUCCEEDED ( pSwapChain->Present ( pCtx->present_man.interval,
                  (pCtx->output.caps.tearing && pCtx->present_man.interval == 0) ? DXGI_PRESENT_ALLOW_TEARING
                                                                                 : 0x0 ) );


            InterlockedIncrement64 (&pCtx->present_man.frames_complete);


            if (bSuccess)
              SetEvent (pCtx->present_man.hAckPresent);
            else
              SetEvent (pCtx->present_man.hAckPresent);

            InterlockedCompareExchange (&lLock, 0, 1);
          }

          if (dwWaitState == PresentEvents::Shutdown)
            break;

        } while (! ReadAcquire (&__SK_DLL_Ending));

        SK_CloseHandle (
          std::exchange (pCtx->present_man.hNotifyPresent, INVALID_HANDLE_VALUE)
        );

        SK_Thread_CloseSelf ();

        return 0;
      }, L"[SK] IndirectX PresentMgr", (LPVOID)pCtx);
  }

  else
  {
    while (ReadAcquire (&lLock) != 0)
      YieldProcessor ();
  }
}

void
SK_IndirectX_PresentManager::Reset (SK_IndirectX_InteropCtx* pCtx)
{
  if (hThread == INVALID_HANDLE_VALUE)
    Start (pCtx);

  SignalObjectAndWait (hNotifyReset, hAckReset, INFINITE, FALSE);
}

void
SK_IndirectX_PresentManager::Present (SK_IndirectX_InteropCtx *pCtx, int Interval)
{
  if (hThread == INVALID_HANDLE_VALUE)
    Start (pCtx);

  this->interval = Interval;

  //SetEvent (hNotifyPresent);
  SignalObjectAndWait (hNotifyPresent, hAckPresent, INFINITE, FALSE);
}

static HWND hwndLast;
static concurrency::concurrent_unordered_map <HDC, SK_IndirectX_InteropCtx>
                                                               _interop_contexts;

#include <../imgui/font_awesome.h>

void
SK_GL_SetVirtualDisplayMode (HWND hWnd, bool Fullscreen, UINT Width, UINT Height)
{
  SK_LOGi1 (
    L"SK_GL_SetVirtualDisplayMode (%x, %s, %lu, %lu)", hWnd, Fullscreen ? L"Fullscreen"
                                                                        : L"Windowed", Width, Height);
  if (Fullscreen)
  {
    RECT                              rect = { };
    GetClientRect (game_window.hWnd, &rect);
    PostMessage (  game_window.hWnd, WM_DISPLAYCHANGE,
                                 32, MAKELPARAM ( rect.right  - rect.left,
                                                  rect.bottom - rect.top ) );
  }
}

void
APIENTRY
SK_GL_InteropDebugOutput (GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, GLvoid *userParam)
{
  auto _DescribeSource = [](GLenum source) -> const wchar_t*
  {
    switch (source)
    {
      case GL_DEBUG_SOURCE_API_ARB:             return L"OpenGL";
      case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:   return L"Window System Layer";
      case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB: return L"Shader Compiler";
      case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:     return L"Third-Party Component";
      case GL_DEBUG_SOURCE_APPLICATION_ARB:     return L"Application Generated";
      case GL_DEBUG_SOURCE_OTHER_ARB:           return L"Other";
      default:                                  return L"Unknown";
    }
  };

  auto _DescribeType = [](GLenum type) -> const wchar_t*
  {
    switch (type)
    {
      case GL_DEBUG_TYPE_ERROR_ARB:               return L"Error";
      case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: return L"Deprecated";
      case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:  return L"Undefined Behavior";
      case GL_DEBUG_TYPE_PORTABILITY_ARB:         return L"Portability";
      case GL_DEBUG_TYPE_PERFORMANCE_ARB:         return L"Performance";
      case GL_DEBUG_TYPE_OTHER_ARB:               return L"Other";
      default:                                    return L"Unknown";
    }
  };

  auto _DescribeSeverity = [](GLenum severity) -> const wchar_t *
  {
    switch (severity)
    {
      case GL_DEBUG_SEVERITY_HIGH_ARB:   return L"High";
      case GL_DEBUG_SEVERITY_MEDIUM_ARB: return L"Medium";
      case GL_DEBUG_SEVERITY_LOW_ARB:    return L"Low";
      default:                           return L"Unknown";
    }
  };

  std::wstring output_msg =
    SK_FormatStringW (
      L"[%ws (%ws) :: %ws]\t%*hs\tObjectId=%d",
      _DescribeSeverity (severity), _DescribeType (type),
      _DescribeSource   (source),   length, message, id
    );

  SK_LOGi0 (L"%ws", output_msg.c_str ());

  if (SK_IsDebuggerPresent ())
  {
    OutputDebugStringW (output_msg.c_str ());

    if (type == GL_DEBUG_TYPE_ERROR_ARB)
    {
      __debugbreak ();
    }
  }

  std::ignore = userParam;
}

HRESULT
SK_GL_CreateInteropSwapChain ( IDXGIFactory2         *pFactory,
                               ID3D11Device          *pDevice, HWND               hWnd,
                               DXGI_SWAP_CHAIN_DESC1 *desc1,   IDXGISwapChain1 **ppSwapChain )
{
  // Disable OpenGL HDR ZeroCopy because of a memory leak on HDR10 codepath
  SK_DXGI_ZeroCopy = false;

  HRESULT hr =
    pFactory->CreateSwapChainForHwnd ( pDevice, hWnd,
                                       desc1,   nullptr,
                                                nullptr, ppSwapChain );

  return hr;
}

void
SK_GL_CheckSRGB (DXGI_FORMAT* fmt = nullptr)
{
  auto& rb =
    SK_GetCurrentRenderBackend ();

  if (rb.active_traits.bOriginallysRGB)
  {
    rb.framebuffer_flags |= SK_FRAMEBUFFER_FLAG_SRGB;
    rb.srgb_stripped      = true;
  
    const bool bIsHDREnabled =
      ( __SK_HDR_10BitSwap ||
        __SK_HDR_16BitSwap );
  
    if (! bIsHDREnabled)
    {
      if (fmt != nullptr)
         *fmt = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
  
      if (config.render.gl.prefer_10bpc)
      {
        config.render.gl.prefer_10bpc = false;
  
        SK_ImGui_Warning (
          L"Game is using an sRGB framebuffer, 10-bpc output has been forced off."
          L"\r\n\r\n\tGamma will be wrong until the game is restarted."
        );
  
        config.utility.save_async ();
      }
    }
  }
}


BOOL
SK_GL_SwapBuffers (HDC hDC, LPVOID pfnSwapFunc)
{
  if (! hDC) // WTF Disgaea?
    return FALSE;

  auto& rb =
    SK_GetCurrentRenderBackend ();

  rb.active_traits.bOriginallysRGB |= static_cast <bool> (glIsEnabled (GL_FRAMEBUFFER_SRGB));
  SK_GL_CheckSRGB ();

  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  auto& pTLS_gl =
    pTLS->render->gl.get ();

  HGLRC& thread_hglrc =
    pTLS_gl.current_hglrc;
  HDC&   thread_hdc   =
    pTLS_gl.current_hdc;


  bool need_init = false;
  {
    std::scoped_lock <SK_Thread_CriticalSection> auto_lock0 (*cs_gl_ctx);

    if (init_->empty () && thread_hglrc == nullptr)
    {
      // This is a nop, it sets the same handles it gets... but it ensures that
      //   other hook libraries are congruent
      wglMakeCurrent ( (thread_hdc   = hDC),//SK_GL_GetCurrentDC      ()),
                       (thread_hglrc = SK_GL_GetCurrentContext ()) );

      pTLS_gl.current_hwnd =
        WindowFromDC (thread_hdc);

      need_init = true;
    }


    if (thread_hglrc != nullptr)
    {
      bool shared_ctx =
        __gl_shared_contexts->count (thread_hglrc) != 0;

      if (init_->count (thread_hglrc) == 0 || need_init)
      {
        // Shared context, and the primary share point is already being tracked
        need_init = !
          (                    shared_ctx &&
            init_->count (__gl_shared_contexts.get ()[thread_hglrc]) );
      }


      if (__gl_primary_context == nullptr)
      {   __gl_primary_context = shared_ctx   ?
          __gl_shared_contexts.get ()[thread_hglrc] : thread_hglrc;
      }


      if (need_init)
      {
        glewExperimental = GL_TRUE;

#ifndef MULTI_CTX_ICD
        SK_RunOnce (glewInit ());
#else
                    glewInit ();
#endif

        init_.get ()[thread_hglrc] = TRUE;

        if (shared_ctx)
          init_.get ()[__gl_shared_contexts.get ()[thread_hglrc]] = TRUE;

        ImGui_ImplGL3_Init ();
      }
    }
  }

  pTLS_gl.current_hdc   = hDC;
  pTLS_gl.current_hwnd  =
           WindowFromDC  (hDC);

//assert (hDC == pTLS->gl.current_hdc);


  BOOL status = false;


  bool compatible_dc =
    (__gl_primary_context == thread_hglrc);

  if (! compatible_dc)
  {
    std::scoped_lock <SK_Thread_CriticalSection> auto_lock1 (*cs_gl_ctx);

    if (__gl_shared_contexts->count (thread_hglrc))
    {
      if (__gl_shared_contexts.get ()[thread_hglrc] == __gl_primary_context)
        compatible_dc = true;
    }

    if (! compatible_dc)
    {
      // Ensure the resources we created on the primary context are meaningful
      //   on this one.
      SK_LOG0 ( ( L"Implicitly sharing lists because Specical K resources were"
                  L" initialized using a different OpenGL context." ),
                  L" OpenGL32 " );

      if (thread_hglrc == nullptr || wglShareLists (__gl_primary_context, thread_hglrc))
      {
        compatible_dc = true;

        if (thread_hglrc == nullptr)
            thread_hglrc = __gl_primary_context;
      }
    }
  }



  if ( ( compatible_dc && init_.get ()[thread_hglrc] ) || config.apis.dxgi.d3d11.hook )
  {
    HWND hWnd =
      WindowFromDC (hDC);// pTLS_gl.current_hdc);
    hwndLast      = hWnd;

    SK_IndirectX_InteropCtx& dx_gl_interop =
                                  _interop_contexts [hDC];

    auto& pDevice    = dx_gl_interop.d3d11.pDevice;
    auto& pDevCtx    = dx_gl_interop.d3d11.pDevCtx;
    auto& pFactory   = dx_gl_interop.d3d11.pFactory;
    auto& pSwapChain = dx_gl_interop.output.pSwapChain;

    RECT                  rcWnd = { };
    GetClientRect (hWnd, &rcWnd);

    HMONITOR hMonitor =
      MonitorFromWindow (hWnd, MONITOR_DEFAULTTONEAREST);

    if (dx_gl_interop.d3d11.pDevice == nullptr)
    {
      static bool glon12 =
        StrStrIA (
          (const char *)glGetString (GL_RENDERER), "D3D12"
        );

      if (config.apis.dxgi.d3d11.hook && (! glon12))
      {
        extern void
        WINAPI SK_HookDXGI (void);
               SK_HookDXGI (    ); // Setup the function to create a D3D11 Device

        SK_ComPtr <IDXGIAdapter>  pAdapter [16] = { nullptr };
        SK_ComPtr <IDXGIFactory7> pFactory7     =   nullptr;

        if (D3D11CreateDeviceAndSwapChain_Import != nullptr)
        {
          if ( SUCCEEDED (
                 CreateDXGIFactory2 ( 0x2,
                                      __uuidof (IDXGIFactory7),
                                          (void **)&pFactory7.p ) )
             )
          {
            pFactory7->QueryInterface <IDXGIFactory2> (&pFactory.p);

            UINT uiAdapterIdx = 0;

            SK_LOG0 ((" DXGI Adapters from Highest to Lowest Perf. "), L"Open GL-IK");
            SK_LOG0 ((" ------------------------------------------ "), L"Open GL-IK");

            while ( SUCCEEDED (
                      pFactory7->EnumAdapterByGpuPreference ( uiAdapterIdx,
                       DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                                    __uuidof (IDXGIAdapter),
                                        (void **)&pAdapter [uiAdapterIdx].p
                  )           )                             )
            {
              DXGI_ADAPTER_DESC         adapter_desc = { };
              pAdapter [uiAdapterIdx]->
                              GetDesc (&adapter_desc);

              SK_LOG0 ( ( L" %d) '%ws'",
                          uiAdapterIdx, adapter_desc.Description ),
                          L"  GLDX11  " );

              ++uiAdapterIdx;
            }
          }

          D3D_FEATURE_LEVEL        levels [] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
                                                 D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };
          D3D_FEATURE_LEVEL featureLevel;

          HRESULT hr =
            DXGI_ERROR_UNSUPPORTED;

          hr =
            D3D11CreateDevice_Import (
              nullptr,//pAdapter [0].p,
                D3D_DRIVER_TYPE_HARDWARE,
                  nullptr,
                    D3D11_CREATE_DEVICE_SINGLETHREADED |
                    D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS,
                      levels,
          _ARRAYSIZE (levels),
                          D3D11_SDK_VERSION,
                            &pDevice.p,
                              &featureLevel,
                                &pDevCtx.p );

          if (FAILED (hr))
          {
            pDevCtx = nullptr;
            pDevice = nullptr;
          }

          if ( SK_ComPtr <IDXGIDevice>
                       pDevDXGI = nullptr ;

              SUCCEEDED (hr)                                                     &&
                        pDevice != nullptr                                       &&
                  ( (pFactory.p != nullptr && pAdapter [0].p != nullptr)         ||
             SUCCEEDED (pDevice->QueryInterface <IDXGIDevice> (&pDevDXGI))       &&
             SUCCEEDED (pDevDXGI->GetAdapter                  (&pAdapter [0].p)) &&
             SUCCEEDED (pAdapter [0]->GetParent (IID_PPV_ARGS (&pFactory)))
                  )
             )
          {
            if (pFactory.p     == nullptr &&
                pAdapter [0].p != nullptr)
            {   pAdapter [0]->GetParent (IID_PPV_ARGS (&pFactory)); }

            SK_GL_OnD3D11 = true;

            SK_ComQIPtr <IDXGIFactory5>
                             pFactory5 (pFactory);

            if (pFactory5 != nullptr)
            {
              SK_ReleaseAssert (
                SUCCEEDED (
                  pFactory5->CheckFeatureSupport (
                    DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                               &dx_gl_interop.output.caps.tearing,
                        sizeof (dx_gl_interop.output.caps.tearing)
                                                 )
                          )
              );

              dx_gl_interop.output.caps.flip_discard = true;
            }

            if (dx_gl_interop.output.caps.flip_discard)
                dx_gl_interop.output.swap_effect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

            if (dx_gl_interop.output.caps.tearing)
            {
              dx_gl_interop.output.swapchain_flags =
                ( DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING |
                  DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT );
            }

            dx_gl_interop.d3d11.featureLevel = featureLevel;

            dx_gl_interop.d3d11.hInteropDevice =
              wglDXOpenDeviceNV (dx_gl_interop.d3d11.pDevice);
          }

          D3D11_BLEND_DESC
            blend_desc                                        = { };
            blend_desc.AlphaToCoverageEnable                  = FALSE;
            blend_desc.RenderTarget [0].BlendEnable           = FALSE;
            blend_desc.RenderTarget [0].SrcBlend              = D3D11_BLEND_ONE;
            blend_desc.RenderTarget [0].DestBlend             = D3D11_BLEND_ZERO;
            blend_desc.RenderTarget [0].BlendOp               = D3D11_BLEND_OP_ADD;
            blend_desc.RenderTarget [0].SrcBlendAlpha         = D3D11_BLEND_ONE;
            blend_desc.RenderTarget [0].DestBlendAlpha        = D3D11_BLEND_ZERO;
            blend_desc.RenderTarget [0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
            blend_desc.RenderTarget [0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

          pDevice->CreateBlendState ( &blend_desc,
                                        &dx_gl_interop.d3d11.pBlendState.p );

          SK_ReleaseAssert (
            SUCCEEDED (
              pDevice->CreatePixelShader ( gl_dx_interop_ps_bytecode,
                                   sizeof (gl_dx_interop_ps_bytecode),
                     nullptr, &dx_gl_interop.d3d11.flipper.pPixelShader.p )
                      )
            );

          SK_ReleaseAssert (
            SUCCEEDED (
              pDevice->CreateVertexShader ( gl_dx_interop_vs_bytecode,
                                    sizeof (gl_dx_interop_vs_bytecode),
                    nullptr, &dx_gl_interop.d3d11.flipper.pVertexShader.p )
                      )
            );

          D3D11_SAMPLER_DESC
            sampler_desc                    = { };

            sampler_desc.Filter             = D3D11_FILTER_MIN_MAG_MIP_POINT;
            sampler_desc.AddressU           = D3D11_TEXTURE_ADDRESS_WRAP;
            sampler_desc.AddressV           = D3D11_TEXTURE_ADDRESS_WRAP;
            sampler_desc.AddressW           = D3D11_TEXTURE_ADDRESS_WRAP;
            sampler_desc.MipLODBias         = 0.f;
            sampler_desc.MaxAnisotropy      =   1;
            sampler_desc.ComparisonFunc     =  D3D11_COMPARISON_NEVER;
            sampler_desc.MinLOD             = -D3D11_FLOAT32_MAX;
            sampler_desc.MaxLOD             =  D3D11_FLOAT32_MAX;

          pDevice->CreateSamplerState ( &sampler_desc,
                                          &dx_gl_interop.d3d11.staging.colorSampler.p );

          D3D11_INPUT_ELEMENT_DESC local_layout [] = {
            { "", 0, DXGI_FORMAT_R32_UINT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
          };

          pDevice->CreateInputLayout ( local_layout, 1,
                                       (void *)(gl_dx_interop_vs_bytecode),
                                        sizeof (gl_dx_interop_vs_bytecode) /
                                        sizeof (gl_dx_interop_vs_bytecode [0]),
                                          &dx_gl_interop.d3d11.flipper.pVertexLayout.p );

          dx_gl_interop.gl.hdc          = hDC;
          dx_gl_interop.gl.hglrc        = thread_hglrc;
          dx_gl_interop.output.hWnd     = hWnd;
          dx_gl_interop.output.hMonitor = MonitorFromWindow (hWnd, MONITOR_DEFAULTTONEAREST);

          dx_gl_interop.stale = true;
        }
      }
    }

    else if (SK_GL_OnD3D11)
    {
      if (hWnd != 0)
      {
        if      (dx_gl_interop.output.hWnd     != hWnd)         dx_gl_interop.stale = true;
        else if (dx_gl_interop.output.hMonitor != hMonitor)     dx_gl_interop.stale = true;
        else if (dx_gl_interop.gl.hdc          != hDC)          dx_gl_interop.stale = true;
        else if (dx_gl_interop.gl.hglrc        != thread_hglrc) dx_gl_interop.stale = true;
        else
        {
          if (! EqualRect (&rcWnd, &dx_gl_interop.output.lastKnownRect))
                                    dx_gl_interop.stale = true;
        }
      }
    }


    extern bool __SK_HDR_AdaptiveToneMap;
    static auto lastAdaptiveToneMapState = !__SK_HDR_AdaptiveToneMap;
    if (std::exchange (lastAdaptiveToneMapState, __SK_HDR_AdaptiveToneMap) != __SK_HDR_AdaptiveToneMap) dx_gl_interop.stale = true;
  //static auto        last16BitHDRState = !__SK_HDR_16BitSwap;
  //static auto        last10BitHDRState = !__SK_HDR_10BitSwap;
  //if (std::exchange (last16BitHDRState, __SK_HDR_16BitSwap) != __SK_HDR_16BitSwap) dx_gl_interop.stale = true;
  //if (std::exchange (last10BitHDRState, __SK_HDR_10BitSwap) != __SK_HDR_10BitSwap) dx_gl_interop.stale = true;


    LONG64 llFrames =
      ReadAcquire64 (&dx_gl_interop.present_man.frames);

    if (llFrames > 0)
    {
      while (ReadAcquire64 (    &dx_gl_interop.present_man.frames_complete) < llFrames)
           WaitForSingleObject ( dx_gl_interop.present_man.hAckPresent, 1);
    }


    if (SK_GL_OnD3D11 && std::exchange (SK_GL_OnD3D11_Reset, false))
                                         dx_gl_interop.stale = true;

    if (SK_GL_OnD3D11 && (std::exchange (dx_gl_interop.stale, false) || pSwapChain == nullptr))
    {
      glFinish ();

      if (pSwapChain != nullptr)
      {
        SK_LOG1 ( ( L" # Recovering from Stale D3D11 Interop Session" ),
                    L"  GLDX11  " );
      }

      SK_GetCurrentRenderBackend ().releaseOwnedResources ();

      if (dx_gl_interop.d3d11.staging.hColorBuffer != nullptr)
      {
        wglDXUnregisterObjectNV (dx_gl_interop.d3d11.hInteropDevice,
                                 dx_gl_interop.d3d11.staging.hColorBuffer);
                                 dx_gl_interop.d3d11.staging.hColorBuffer = nullptr;
      }

      IUnknown_AtomicRelease ((void **)&dx_gl_interop.d3d11.staging.colorView.p);
      IUnknown_AtomicRelease ((void **)&dx_gl_interop.d3d11.staging.colorBuffer.p);
      IUnknown_AtomicRelease ((void **)&dx_gl_interop.output.backbuffer.image.p);
      IUnknown_AtomicRelease ((void **)&dx_gl_interop.output.backbuffer.rtv.p);

      if (                    0 != dx_gl_interop.gl.fbo) {
        glDeleteFramebuffers  (1, &dx_gl_interop.gl.fbo);
                                   dx_gl_interop.gl.fbo = 0;
      }

      if (                    0 != dx_gl_interop.gl.color_rbo) {
        glDeleteRenderbuffers (1, &dx_gl_interop.gl.color_rbo);
                                   dx_gl_interop.gl.color_rbo = 0;
      }

      dx_gl_interop.output.lastKnownRect = rcWnd;

      auto w = dx_gl_interop.output.lastKnownRect.right  - dx_gl_interop.output.lastKnownRect.left;
      auto h = dx_gl_interop.output.lastKnownRect.bottom - dx_gl_interop.output.lastKnownRect.top;

      dx_gl_interop.output.hWnd     = hWnd;
      dx_gl_interop.output.hMonitor = hMonitor;
      dx_gl_interop.gl.hdc          = hDC;
      dx_gl_interop.gl.hglrc        = thread_hglrc;

      DXGI_SWAP_CHAIN_DESC1
        desc1                    = { };
        desc1.Format             = config.render.gl.prefer_10bpc ? DXGI_FORMAT_R10G10B10A2_UNORM :
                                                                   DXGI_FORMAT_R8G8B8A8_UNORM;
        desc1.SampleDesc.Count   = 1;
        desc1.SampleDesc.Quality = 0;
        desc1.Width              = w;
        desc1.Height             = h;
        desc1.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT |
                                   DXGI_USAGE_SHADER_INPUT         |
                                   DXGI_USAGE_BACK_BUFFER          |
        ( dx_gl_interop.d3d11.featureLevel >= D3D_FEATURE_LEVEL_11_0  ?
                                   DXGI_USAGE_UNORDERED_ACCESS        : 0x0 );
        desc1.BufferCount        = _DXBackBuffers;
        desc1.AlphaMode          = DXGI_ALPHA_MODE_IGNORE;
        desc1.Scaling            = DXGI_SCALING_NONE;
        desc1.SwapEffect         = dx_gl_interop.output.swap_effect;
        desc1.Flags              = dx_gl_interop.output.swapchain_flags;

      if (                             dx_gl_interop.output.hSemaphore != nullptr)
        SK_CloseHandle (std::exchange (dx_gl_interop.output.hSemaphore,   nullptr));

      // HDR Format Overrides (independent from SDR bit depth preference)
      if (     __SK_HDR_16BitSwap)
        desc1.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
      else if (__SK_HDR_10BitSwap)
        desc1.Format = DXGI_FORMAT_R10G10B10A2_UNORM;

      if (std::exchange (dx_gl_interop.output.hWnd, hWnd) != hWnd || pSwapChain == nullptr)
      {
        pSwapChain = nullptr;

        SK_GL_CheckSRGB (&desc1.Format);

        SK_GL_CreateInteropSwapChain ( pFactory, dx_gl_interop.d3d11.pDevice,
                                         dx_gl_interop.output.hWnd,
                                           &desc1, &pSwapChain.p );

        pFactory->MakeWindowAssociation ( dx_gl_interop.output.hWnd,
                                            DXGI_MWA_NO_ALT_ENTER |
                                            DXGI_MWA_NO_WINDOW_CHANGES );
      }

      // Dimensions were wrong, SwapChain just needs a resize...
      else
      {
        // Originally, DXGI_FORMAT_UNKNOWN was used here, but that causes
        //   problems with NVIDIA's driver believing that surface sharing
        //     is failing parameter validation.  * Be explicit about it!
        pSwapChain->ResizeBuffers (
                   _DXBackBuffers, w, h,
                    desc1.Format, dx_gl_interop.output.swapchain_flags
        );
      }

      pSwapChain->GetDesc1 (&desc1);

      dx_gl_interop.output.viewport = { 0.0f, 0.0f,
                     static_cast <float> (desc1.Width),
                     static_cast <float> (desc1.Height),
                                        0.0f, 1.0f
      };

      SK_ComQIPtr <IDXGISwapChain2>
          pSwapChain2 (pSwapChain);
      if (pSwapChain2 != nullptr) dx_gl_interop.output.hSemaphore =
          pSwapChain2->GetFrameLatencyWaitableObject ();


      bool bHDRZeroCopy =
        (__SK_HDR_16BitSwap||__SK_HDR_10BitSwap) && !__SK_HDR_AdaptiveToneMap;

      if (bHDRZeroCopy)
      {
        SK_ComPtr <ID3D11ShaderResourceView> pSrv;
        SK_ComPtr <ID3D11Resource>           pRes;

        IDXGISwapChain*                                 pWrappedSwapChain = nullptr;
        SK_DXGI_GetPrivateData ( pSwapChain,
          SKID_DXGI_WrappedSwapChain, sizeof (void *), &pWrappedSwapChain );

        SK_DXGI_GetPrivateData ( pWrappedSwapChain,
          SKID_DXGI_SwapChainProxyBackbuffer_D3D11, sizeof (void *), &pSrv.p
        );

        if (pSrv != nullptr)
        {   pSrv->GetResource (&pRes.p);

          SK_ComQIPtr <ID3D11Texture2D> pTex2D (pRes);

          if (pTex2D != nullptr)
          {
            dx_gl_interop.output.backbuffer.image = pRes;
            dx_gl_interop.output.backbuffer.rtv   = nullptr;

            D3D11_TEXTURE2D_DESC texDesc = { };
            pTex2D->GetDesc    (&texDesc);

            D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = { };
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            rtvDesc.Format        = DirectX::MakeTypelessUNORM (
                                    DirectX::MakeTypelessFLOAT (texDesc.Format));

            pDevice->CreateRenderTargetView (   dx_gl_interop.output.backbuffer.image.p,
                                     &rtvDesc, &dx_gl_interop.output.backbuffer.rtv.p   );
          }
        }

        if (! dx_gl_interop.output.backbuffer.rtv.p)
        {
          dx_gl_interop.output.backbuffer.image = nullptr;
          bHDRZeroCopy                          = false;
        }
      }

      if (! bHDRZeroCopy)
      {
        dx_gl_interop.output.backbuffer.image = nullptr;
        dx_gl_interop.output.backbuffer.rtv   = nullptr;

        pSwapChain->GetBuffer (
          0, IID_ID3D11Texture2D, (void **)&dx_gl_interop.output.backbuffer.image.p );
        pDevice->CreateRenderTargetView (   dx_gl_interop.output.backbuffer.image.p,
                                  nullptr, &dx_gl_interop.output.backbuffer.rtv.p   );
      }

      // We can skip the vertical flip operation if SK's HDR mode is enabled
      if (! bHDRZeroCopy)
      {
        D3D11_TEXTURE2D_DESC                             tex_desc = { };
        dx_gl_interop.output.backbuffer.image->GetDesc (&tex_desc);

        tex_desc.Format = DirectX::MakeTypelessUNORM (
                          DirectX::MakeTypelessFLOAT (tex_desc.Format));

        tex_desc.ArraySize          = 1;
        tex_desc.MipLevels          = 1;
        tex_desc.SampleDesc.Count   = 1;
        tex_desc.SampleDesc.Quality = 0;
        tex_desc.Usage              = D3D11_USAGE_DEFAULT;
        tex_desc.BindFlags          = D3D11_BIND_RENDER_TARGET |
                                      D3D11_BIND_SHADER_RESOURCE |
                                      D3D11_BIND_UNORDERED_ACCESS;
        tex_desc.CPUAccessFlags     = 0;
        tex_desc.MiscFlags          = D3D11_RESOURCE_MISC_SHARED;

        dx_gl_interop.d3d11.staging.colorBuffer = nullptr;
        dx_gl_interop.d3d11.staging.colorView   = nullptr;

        pDevice->CreateTexture2D (         &tex_desc,                                  nullptr,
                                           &dx_gl_interop.d3d11.staging.colorBuffer.p);
        pDevice->CreateShaderResourceView ( dx_gl_interop.d3d11.staging.colorBuffer.p, nullptr,
                                           &dx_gl_interop.d3d11.staging.colorView.p);
      }

      else // Skip the vertical flip!
      {
        dx_gl_interop.d3d11.staging.colorView   = nullptr;
        dx_gl_interop.d3d11.staging.colorBuffer = dx_gl_interop.output.backbuffer.image;
      }

      dx_gl_interop.present_man.Reset (&dx_gl_interop);


      if (dx_gl_interop.d3d11.staging.colorBuffer.p != nullptr)
      {
        glGenRenderbuffers (1, &dx_gl_interop.gl.color_rbo);
        glGenFramebuffers  (1, &dx_gl_interop.gl.fbo);

        dx_gl_interop.d3d11.staging.hColorBuffer =
          wglDXRegisterObjectNV ( dx_gl_interop.d3d11.hInteropDevice,
                                  dx_gl_interop.d3d11.staging.colorBuffer,
                                  dx_gl_interop.gl.color_rbo, GL_RENDERBUFFER,
                                                              WGL_ACCESS_WRITE_DISCARD_NV );
      }
    }


    if (dx_gl_interop.d3d11.staging.colorBuffer == nullptr || (! SK_GL_OnD3D11))
    {
      if (! SK_GL_OnD3D11)
      {
        SK_RunOnce (
          SK_ImGui_WarningWithTitle (
            L"Please turn Direct3D 11 on under Compatibility Settings | Render Backends "
            L"and restart the game", L"Pure OpenGL Is No Longer Supported!"
          )
        );
      }

      SK_GetCurrentRenderBackend ().api =
        SK_RenderAPI::OpenGL;

      SK_GL_UpdateRenderStats ();
      SK_Overlay_DrawGL       ();

      // Do this before framerate limiting
      glFlush                 ();
      SK_BeginBufferSwap      ();
    }

    else
    {
      wglDXLockObjectsNV   ( dx_gl_interop.d3d11.hInteropDevice, 1,
                            &dx_gl_interop.d3d11.staging.hColorBuffer );

      GLint                                   original_scissor;
      GLint                                   original_fbo;
      glGetIntegerv (GL_FRAMEBUFFER_BINDING, &original_fbo);
      glGetIntegerv (GL_SCISSOR_TEST,        &original_scissor);

      glBindFramebuffer         (GL_FRAMEBUFFER,  dx_gl_interop.gl.fbo);
      glFramebufferRenderbuffer (GL_FRAMEBUFFER,  GL_COLOR_ATTACHMENT0,
                                 GL_RENDERBUFFER, dx_gl_interop.gl.color_rbo);

      glBindFramebuffer (GL_READ_FRAMEBUFFER, 0);
      glBindFramebuffer (GL_DRAW_FRAMEBUFFER, dx_gl_interop.gl.fbo);

      glDisable         (GL_SCISSOR_TEST);

      GLint w = static_cast <GLint> (dx_gl_interop.output.viewport.Width),
            h = static_cast <GLint> (dx_gl_interop.output.viewport.Height);

      // Don't do this if the game is minimized :)
      if (w > 0 && h > 0)
      {
        GLenum framebufferStatus =  glCheckFramebufferStatus (GL_DRAW_FRAMEBUFFER);
        if (   framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
          SK_LOG0 ( ( L"FBO Status: %x", framebufferStatus ), L"  GLDX11  " );

        if (glClampColor != nullptr)
            glClampColor (GL_CLAMP_READ_COLOR, GL_FIXED_ONLY);

        bool fallback = false;

        if (dx_gl_interop.gl.fullscreen)
        {
          glGetError ();

          UINT halfX = (w - dx_gl_interop.gl.width);
          UINT halfY = (h - dx_gl_interop.gl.height);

          if (halfX > 1) halfX /= 2;
          if (halfY > 1) halfY /= 2;

          glBlitFramebuffer (     0,0,     dx_gl_interop.gl.width, dx_gl_interop.gl.height,
                              halfX,halfY,                w-halfX, h-halfY,
                                                       GL_COLOR_BUFFER_BIT, GL_NEAREST );

          if (glGetError () != GL_NO_ERROR)
          {
            fallback = true;
            SK_RunOnce (SK_ImGui_Warning (L"Fullscreen Emulation Fallback Triggered"));
          }
        }

        if (fallback || (! dx_gl_interop.gl.fullscreen))
        {
          glGetError ();

          glBlitFramebuffer ( 0,0, w,h,
                              0,0, w,h, GL_COLOR_BUFFER_BIT, GL_NEAREST );
        }

        GLenum err = glGetError ();

        if (err != GL_NO_ERROR)
          SK_LOG0 ( ( L"glBlitFramebuffer Error: %x", err ), L"  GLDX11  " );
      }

      glBindFramebuffer (GL_FRAMEBUFFER,       original_fbo);
      if (glClampColor != nullptr)
          glClampColor  (GL_CLAMP_READ_COLOR, GL_FIXED_ONLY);
      glFlush           (                                  );

      if (original_scissor == GL_TRUE)
        glEnable (GL_SCISSOR_TEST);

      wglDXUnlockObjectsNV ( dx_gl_interop.d3d11.hInteropDevice, 1,
                            &dx_gl_interop.d3d11.staging.hColorBuffer );
    }

    bool _SkipThisFrame = false;


    // Time how long the swap actually takes, because
    //   various hooked third-party overlays may be
    //     adding extra CPU time to each swap
    SK_LatentSync_BeginSwap ();


    if (config.render.framerate.present_interval == 0 &&
        config.render.framerate.target_fps        > 0.0f)
    {
      if (__SK_LatentSyncSkip != 0 && (__SK_LatentSyncFrame % __SK_LatentSyncSkip) != 0)
        _SkipThisFrame = true;

      if (_SkipThisFrame)
      {
        //BOOL SK_GL_InsertDuplicateFrame (void);
        //     SK_GL_InsertDuplicateFrame ();
      }
    }

    if (! _SkipThisFrame)
    {
      if (SK_GL_OnD3D11 && pSwapChain != nullptr)
      {
        int present_interval =
          SK_GL_GetSwapInterval ();

        if (config.render.framerate.present_interval != SK_NoPreference)
          present_interval = config.render.framerate.present_interval;

        present_interval =
          std::min (4, std::max (0, present_interval));

        if (                                             config.render.framerate.sync_interval_clamp > 0)
          present_interval = std::min (present_interval, config.render.framerate.sync_interval_clamp);


        dx_gl_interop.present_man.Present (&dx_gl_interop, present_interval);
      }

      else
      {
        if (__SK_BFI)
          SK_GL_SwapInterval (0);

        status =
          static_cast_pfn <wglSwapBuffers_pfn> (pfnSwapFunc)(hDC);
      }

    }
    else
      status = TRUE;

    // This info is used to dynamically adjust target
    //   sync time for laser precision
    SK_LatentSync_EndSwap ();

    if (! SK_GL_OnD3D11)
    {
      if (status)
        SK_EndBufferSwap (S_OK);
      else
        SK_EndBufferSwap (E_UNEXPECTED);

      if (config.render.framerate.present_interval == 0 &&
          config.render.framerate.target_fps        > 0.0f)
      {
        if (__SK_BFI)
        {
          ////extern LONGLONG __SK_LatentSync_FrameInterval;
          ////
          ////static SK_AutoHandle hTimer (INVALID_HANDLE_VALUE);
          ////
          ////extern void SK_Framerate_WaitUntilQPC (LONGLONG llQPC, HANDLE& hTimer);

          ////SK_Framerate_WaitUntilQPC (
          ////  SK_QueryPerf ().QuadPart + __SK_LatentSync_FrameInterval,
          ////               hTimer.m_h );

          SK_GL_GhettoStateBlock_Capture ();

          glBindFramebuffer (GL_FRAMEBUFFER, 0);
          glDisable         (GL_FRAMEBUFFER_SRGB);

          glColorMask       (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
          glDrawBuffer      (GL_BACK_LEFT);

          glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
          glClear      (GL_COLOR_BUFFER_BIT/* | GL_DEPTH_BUFFER_BIT
                                              | GL_STENCIL_BUFFER_BIT*/);

          SK_GL_GhettoStateBlock_Apply ();

          SK_GetCurrentRenderBackend ().api =
            SK_RenderAPI::OpenGL;

          SK_GL_UpdateRenderStats ();
          SK_Overlay_DrawGL       ();

          // Do this before framerate limiting
          glFlush                 ();

          SK_BeginBufferSwap      ();

          // Time how long the swap actually takes, because
          //   various hooked third-party overlays may be
          //     adding extra CPU time to each swap
          SK_LatentSync_BeginSwap ();

          SK_GL_SwapInterval (1);
          status =
            static_cast_pfn <wglSwapBuffers_pfn> (pfnSwapFunc)(hDC);
          SK_GL_SwapInterval (0);

          // This info is used to dynamically adjust target
          //   sync time for laser precision
          SK_LatentSync_EndSwap ();

          if (status)
            SK_EndBufferSwap (S_OK);
          else
            SK_EndBufferSwap (E_UNEXPECTED);
        }
      }

      SK_Screenshot_ProcessQueue (SK_ScreenshotStage::EndOfFrame,    SK_GetCurrentRenderBackend ());
      SK_Screenshot_ProcessQueue (SK_ScreenshotStage::ClipboardOnly, SK_GetCurrentRenderBackend ());
      SK_Screenshot_ProcessQueue (SK_ScreenshotStage::_FlushQueue,   SK_GetCurrentRenderBackend ());
    }
  }

  // Swap happening on a context we don't care about
  else
  {
    status =
      static_cast_pfn <wglSwapBuffers_pfn> (pfnSwapFunc)(hDC);
  }

  return status;
}


void
SK_GL_TrackHDC (HDC hDC)
{
  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  HWND hWnd_DC =
    WindowFromDC (hDC);

  HWND hWnd_Root =
    GetAncestor (hWnd_DC, GA_ROOT);

#if 0
  if (rb.windows.device != hWnd_DC && SK_Win32_IsGUIThread ())
  {
    extern HWND WINAPI SK_GetFocus (void);

    if (IsWindowVisible (hWnd_DC) && SK_GetFocus () == hWnd_DC)
    {
      SK_InstallWindowHook (game_window.hWnd/*GetActiveWindow ()*/);

      if (game_window.WndProc_Original != nullptr)
        rb.windows.setDevice (hWnd_DC);
    }
  }
#else
  auto& windows =
      rb.windows;

  if (game_window.hWnd != hWnd_Root)
  {
    if ( windows.device != nullptr &&
                hWnd_DC != nullptr &&
                hWnd_DC != windows.device )
    {
      dll_log->Log (L"[   DXGI   ] Game created a new window?!");

      windows.setDevice (hWnd_DC);
      windows.setFocus  (hWnd_Root);
    }

    else
    {
      SK_InstallWindowHook (hWnd_Root);

      windows.setDevice (hWnd_DC);
      windows.setFocus  (hWnd_Root);
    }
  }

  SK_Inject_SetFocusWindow (hWnd_Root);
#endif
}

//
// SwapBufers (...) in gdi32.dll calls through wglSwapBuffers -- the appropriate
//                    place to change hooks is at the end of this function because
//                      it comes at the very end of a frame.
//
__declspec (noinline)
BOOL
WINAPI
wglSwapBuffers (HDC hDC)
{
  WaitForInit_GL ();

  SK_LOG1 ( ( L"[%x (tid=%04x)]  wglSwapBuffers (hDC=%x)", WindowFromDC (hDC), SK_Thread_GetCurrentId (), hDC ),
              L" OpenGL32 " );

  SK_GL_TrackHDC (hDC);


  BOOL bRet = FALSE;

  int orig_swap_interval =
    SK_GL_GetSwapInterval ();

  // Sync Interval Clamp  (NOTE: SyncInterval > 1 Disables VRR)
  //
  if ( config.render.framerate.sync_interval_clamp != SK_NoPreference &&
       config.render.framerate.sync_interval_clamp < SK_GL_GetSwapInterval () )
  {
    SK_GL_SwapInterval (config.render.framerate.sync_interval_clamp);
  }

  if (                  config.render.framerate.present_interval != SK_NoPreference)
    SK_GL_SwapInterval (config.render.framerate.present_interval);

  if (! config.render.osd.draw_in_vidcap)
    bRet = SK_GL_SwapBuffers (hDC, static_cast_pfn <LPVOID> (wgl_swap_buffers));

  else
    bRet = wgl_swap_buffers (hDC);

  config.render.osd._last_vidcap_frame = SK_GetFramesDrawn ();

  SK_GL_SwapInterval (orig_swap_interval);


  return bRet;
}

__declspec (noinline)
BOOL
WINAPI
SwapBuffers (HDC hDC)
{
  WaitForInit_GL ();

  SK_LOG1 ( ( L"[%x (tid=%04x)]  SwapBuffers (hDC=%x)", WindowFromDC (hDC), SK_Thread_GetCurrentId (), hDC ),
              L" OpenGL32 " );

  SK_GL_TrackHDC (hDC);


  config.render.osd._last_normal_frame = SK_GetFramesDrawn ();

  BOOL bRet = FALSE;

  int orig_swap_interval =
    SK_GL_GetSwapInterval ();

  if (                  config.render.framerate.present_interval != SK_NoPreference)
    SK_GL_SwapInterval (config.render.framerate.present_interval);

  if (config.render.osd.draw_in_vidcap)
    bRet = SK_GL_SwapBuffers (hDC, static_cast_pfn <LPVOID> (gdi_swap_buffers));

  else
    bRet = gdi_swap_buffers (hDC);

  SK_GL_SwapInterval (orig_swap_interval);


  return bRet;
}

HGLRC
WINAPI
wglCreateContextAttribsARB_SK (HDC hDC, HGLRC hshareContext, const int *attribList)
{
  SK_LOG_FIRST_CALL

  ++SK_GL_ContextCount;

  return
    wgl_create_context_attribs (hDC, hshareContext, attribList);
}


//typedef struct _WGLSWAP
//{
//  HDC  hDC;
//  UINT uiFlags;
//} WGLSWAP;


typedef DWORD (WINAPI *wglSwapMultipleBuffers_pfn)(UINT n, CONST WGLSWAP *ps);
                       wglSwapMultipleBuffers_pfn
                       wgl_swap_multiple_buffers = nullptr;

__declspec (noinline)
DWORD
WINAPI
wglSwapMultipleBuffers (UINT n, const WGLSWAP* ps)
{
  if (ps == nullptr)
    return 0;

  WaitForInit_GL ();

  SK_LOG0 ( ( L"wglSwapMultipleBuffers [%lu]", n ), L" OpenGL32 " );

  DWORD dwTotal = 0;
  DWORD dwRet   = 0;

  for (UINT i = 0; i < n; i++)
  {
    dwRet    = SwapBuffers (ps [i].hdc);
    dwTotal += dwRet;
  }

  return dwTotal;
}



#define GL_QUERY_RESULT           0x8866
#define GL_QUERY_RESULT_AVAILABLE 0x8867
#define GL_QUERY_RESULT_NO_WAIT   0x9194

#define GL_TIMESTAMP              0x8E28
#define GL_TIME_ELAPSED           0x88BF

typedef int64_t  GLint64;
typedef uint64_t GLuint64;

typedef GLvoid    (WINAPI *glGenQueries_pfn)   (GLsizei n,       GLuint *ids);
typedef GLvoid    (WINAPI *glDeleteQueries_pfn)(GLsizei n, const GLuint *ids);

typedef GLvoid    (WINAPI *glBeginQuery_pfn)       (GLenum target, GLuint id);
typedef GLvoid    (WINAPI *glBeginQueryIndexed_pfn)(GLenum target, GLuint index, GLuint id);
typedef GLvoid    (WINAPI *glEndQuery_pfn)         (GLenum target);

typedef GLboolean (WINAPI *glIsQuery_pfn)            (GLuint id);
typedef GLvoid    (WINAPI *glQueryCounter_pfn)       (GLuint id, GLenum target);
typedef GLvoid    (WINAPI *glGetQueryObjectiv_pfn)   (GLuint id, GLenum pname, GLint    *params);
typedef GLvoid    (WINAPI *glGetQueryObjecti64v_pfn) (GLuint id, GLenum pname, GLint64  *params);
typedef GLvoid    (WINAPI *glGetQueryObjectui64v_pfn)(GLuint id, GLenum pname, GLuint64 *params);

#if 0
glGenQueries_pfn          glGenQueries;
glDeleteQueries_pfn       glDeleteQueries;

glBeginQuery_pfn          glBeginQuery;
glBeginQueryIndexed_pfn   glBeginQueryIndexed;
glEndQuery_pfn            glEndQuery;

glIsQuery_pfn             glIsQuery;
glQueryCounter_pfn        glQueryCounter;
glGetQueryObjectiv_pfn    glGetQueryObjectiv;
glGetQueryObjecti64v_pfn  glGetQueryObjecti64v;
glGetQueryObjectui64v_pfn glGetQueryObjectui64v;
#endif

namespace GLPerf
{
  bool Init     (void);
  bool Shutdown (void);

  void StartFrame (void);
  void EndFrame   (void);

  class PipelineQuery {
  public:
    PipelineQuery (const wchar_t* wszName, GLenum target) : name_ (wszName)
    {
      glGenQueries (1, &query_);

      finished_  = GL_FALSE;
      ready_     = GL_TRUE;

      target_    = target;
      active_    = GL_FALSE;

      result_    = 0ULL;
    }

   ~PipelineQuery (GLvoid)
   {
      if (glIsQuery (query_))
      {
        glDeleteQueries (1, &query_);
        query_ = 0;
      }
    }

    std::wstring getName         (GLvoid) const
    {
      return name_;
    }

    GLvoid    beginQuery         (GLvoid)
    {
      active_   = GL_TRUE;
      ready_    = GL_FALSE;
      glBeginQuery (target_, query_);
      finished_ = GL_FALSE;
      result_   = 0;
    }
    GLvoid    endQuery           (GLvoid)
    {
      active_   = GL_FALSE;
      glEndQuery (query_);
    }

    GLboolean isFinished         (GLvoid)
    {
      GLint finished;

      glGetQueryObjectiv (query_, GL_QUERY_RESULT_AVAILABLE, &finished);

      finished_ = (finished > 0);

      if (finished_)
        return GL_TRUE;

      return GL_FALSE;
    }
    GLboolean isReady            (GLvoid) const
    {
      return ready_;
    }
    GLboolean isActive           (GLvoid) const
    {
      return active_;
    }

    // Will return immediately if not ready
    GLboolean getResulIfFinished (GLuint64* pResult)
    {
      if (isFinished ())
      {
        getResult (pResult);
        return GL_TRUE;
      }

      return GL_FALSE;
    }

    // Will BLOCK
    GLvoid    getResult          (GLuint64* pResult)
    {
      glGetQueryObjectui64v (query_, GL_QUERY_RESULT, pResult);
      ready_ = GL_TRUE;

      result_ = *pResult;
    }

    GLuint64 getLastResult (void) const
    {
      return result_;
    }

  protected:
    GLboolean    active_   = GL_FALSE;
    GLboolean    finished_ = GL_FALSE; // Has GL given us the data yet?
    GLboolean    ready_    = GL_FALSE; // Has the old value has been retrieved yet?

    GLuint64     result_   = 0ULL;     // Cached result

  private:
    GLuint       query_    = 0UL;     // GL Name of Query Object
    GLenum       target_   = GL_NONE; // Target;
    std::wstring name_     = L"";     // Human-readable name
  };

  bool HAS_pipeline_query;

  enum {
    IAVertices    = 0,
    IAPrimitives  = 1,
    VSInvocations = 2,
    GSInvocations = 3,
    GSPrimitives  = 4,
    CInvocations  = 5,
    CPrimitives   = 6,
    PSInvocations = 7,
    HSInvocations = 8,
    DSInvocations = 9,
    CSInvocations = 10,

    PipelineStateCount
  };

  PipelineQuery* pipeline_states [PipelineStateCount];
}

#define GL_VERTICES_SUBMITTED_ARB                          0x82EE
#define GL_PRIMITIVES_SUBMITTED_ARB                        0x82EF
#define GL_VERTEX_SHADER_INVOCATIONS_ARB                   0x82F0
#define GL_TESS_CONTROL_SHADER_PATCHES_ARB                 0x82F1
#define GL_TESS_EVALUATION_SHADER_INVOCATIONS_ARB          0x82F2
#define GL_GEOMETRY_SHADER_INVOCATIONS                     0x887F
#define GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB          0x82F3
#define GL_FRAGMENT_SHADER_INVOCATIONS_ARB                 0x82F4
#define GL_COMPUTE_SHADER_INVOCATIONS_ARB                  0x82F5
#define GL_CLIPPING_INPUT_PRIMITIVES_ARB                   0x82F6
#define GL_CLIPPING_OUTPUT_PRIMITIVES_ARB                  0x82F7

bool
GLPerf::Init (void)
{
  glGenQueries =
    (glGenQueries_pfn)wglGetProcAddress ("glGenQueries");

  glDeleteQueries =
    (glDeleteQueries_pfn)wglGetProcAddress ("glDeleteQueries");

  glBeginQuery =
    (glBeginQuery_pfn)wglGetProcAddress ("glBeginQuery");

  glBeginQueryIndexed =
    (glBeginQueryIndexed_pfn)wglGetProcAddress ("glBeginQueryIndexed");

  glEndQuery =
    (glEndQuery_pfn)wglGetProcAddress ("glEndQuery");

  glIsQuery =
    (glIsQuery_pfn)wglGetProcAddress ("glIsQuery");

  glQueryCounter =
    (glQueryCounter_pfn)wglGetProcAddress ("glQueryCounter");

  glGetQueryObjectiv =
    (glGetQueryObjectiv_pfn)wglGetProcAddress ("glGetQueryObjectiv");

  glGetQueryObjecti64v =
    (glGetQueryObjecti64v_pfn)wglGetProcAddress ("glGetQueryObjecti64v");

  glGetQueryObjectui64v =
    (glGetQueryObjectui64v_pfn)wglGetProcAddress ("glGetQueryObjectui64v");

  HAS_pipeline_query =
    glGenQueries && glDeleteQueries && glBeginQuery       && glBeginQueryIndexed  && glEndQuery &&
    glIsQuery    && glQueryCounter  && glGetQueryObjectiv && glGetQueryObjecti64v && glGetQueryObjectui64v;

  if (HAS_pipeline_query)
  {
    pipeline_states [ 0] = new PipelineQuery (L"Vertices Submitted",                         GL_VERTICES_SUBMITTED_ARB);
    pipeline_states [ 1] = new PipelineQuery (L"Primitives Submitted",                       GL_PRIMITIVES_SUBMITTED_ARB);
    pipeline_states [ 2] = new PipelineQuery (L"Vertex Shader Invocations",                  GL_VERTEX_SHADER_INVOCATIONS_ARB);
    pipeline_states [ 3] = new PipelineQuery (L"Tessellation Control Shader Patches",        GL_TESS_CONTROL_SHADER_PATCHES_ARB);
    pipeline_states [ 4] = new PipelineQuery (L"Tessellation Evaluation Shader Invocations", GL_TESS_EVALUATION_SHADER_INVOCATIONS_ARB);
    pipeline_states [ 5] = new PipelineQuery (L"Geometry Shader Invocations",                GL_GEOMETRY_SHADER_INVOCATIONS);
    pipeline_states [ 6] = new PipelineQuery (L"Geometry Shader Primitives Emitted",         GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB);
    pipeline_states [ 7] = new PipelineQuery (L"Fragment Shader Invocations",                GL_FRAGMENT_SHADER_INVOCATIONS_ARB);
    pipeline_states [ 8] = new PipelineQuery (L"Compute Shader Invocations",                 GL_COMPUTE_SHADER_INVOCATIONS_ARB);
    pipeline_states [ 9] = new PipelineQuery (L"Clipping Input Primitives",                  GL_CLIPPING_INPUT_PRIMITIVES_ARB);
    pipeline_states [10] = new PipelineQuery (L"Clipping Output Primitives",                 GL_CLIPPING_OUTPUT_PRIMITIVES_ARB);

    return true;
  }

  return false;
}


static bool _init_render_stats = false;

void
__stdcall
SK_GL_UpdateRenderStats (void)
{
  if (! (config.render.show))
    return;

  if (! _init_render_stats)
  {
    GLPerf::Init ();
    _init_render_stats = true;
  }

  //SK::DXGI::PipelineStatsD3D11& pipeline_stats =
    //SK::DXGI::pipeline_stats_d3d11;

  for (auto & pipeline_state : GLPerf::pipeline_states)
  {
    if (pipeline_state != nullptr)
    {
      if (pipeline_state->isReady ())
      {
        GLuint64 result;
        if (pipeline_state->getResulIfFinished (&result))
        {
          pipeline_state->beginQuery ();
        }
      }

      else if (pipeline_state->isActive ())
      {
        pipeline_state->endQuery ();
      }

      //if (! GLPerf::pipeline_states [i]->isActive ())
      //{
      //
      //}
    }
  }
}

std::string
SK::OpenGL::getPipelineStatsDesc (void)
{
  char szDesc [1024] = { };

  D3D11_QUERY_DATA_PIPELINE_STATISTICS stats = { };

  if (GLPerf::HAS_pipeline_query)
  {
    stats.IAVertices    = GLPerf::pipeline_states [GLPerf::IAVertices   ]->getLastResult ();
    stats.IAPrimitives  = GLPerf::pipeline_states [GLPerf::IAPrimitives ]->getLastResult ();
    stats.VSInvocations = GLPerf::pipeline_states [GLPerf::VSInvocations]->getLastResult ();
    stats.GSInvocations = GLPerf::pipeline_states [GLPerf::GSInvocations]->getLastResult ();
    stats.GSPrimitives  = GLPerf::pipeline_states [GLPerf::GSPrimitives ]->getLastResult ();
    stats.CInvocations  = GLPerf::pipeline_states [GLPerf::CInvocations ]->getLastResult ();
    stats.CPrimitives   = GLPerf::pipeline_states [GLPerf::CPrimitives  ]->getLastResult ();
    stats.PSInvocations = GLPerf::pipeline_states [GLPerf::PSInvocations]->getLastResult ();
    stats.HSInvocations = GLPerf::pipeline_states [GLPerf::HSInvocations]->getLastResult ();
    stats.DSInvocations = GLPerf::pipeline_states [GLPerf::DSInvocations]->getLastResult ();
    stats.CSInvocations = GLPerf::pipeline_states [GLPerf::CSInvocations]->getLastResult ();
  }

  //
  // VERTEX SHADING
  //
  if (stats.VSInvocations > 0)
  {
    sprintf ( szDesc,
                 "  VERTEX : %s   (%s Verts ==> %s Triangles)\n",
                   SK_CountToString (stats.VSInvocations).c_str (),
                     SK_CountToString (stats.IAVertices).c_str (),
                       SK_CountToString (stats.IAPrimitives).c_str () );
  }

  else
  {
    sprintf ( szDesc,
                 "  VERTEX : <Unused>\n" );
  }

  //
  // GEOMETRY SHADING
  //
  if (stats.GSInvocations > 0)
  {
    sprintf ( szDesc,
                 "%s  GEOM   : %s   (%s Prims)\n",
                   std::string (szDesc).c_str (),
                     SK_CountToString (stats.GSInvocations).c_str (),
                       SK_CountToString (stats.GSPrimitives).c_str () );
  }

  else
  {
    sprintf ( szDesc,
                 "%s  GEOM   : <Unused>\n",
                   std::string (szDesc).c_str () );
  }

  //
  // TESSELLATION
  //
  if (stats.HSInvocations > 0 || stats.DSInvocations > 0)
  {
    sprintf ( szDesc,
                 "%s  TESS   : %s Hull ==> %s Domain\n",
                   std::string (szDesc).c_str (),
                     SK_CountToString (stats.HSInvocations).c_str (),
                       SK_CountToString (stats.DSInvocations).c_str () ) ;
  }

  else
  {
    sprintf ( szDesc,
                 "%s  TESS   : <Unused>\n",
                   std::string (szDesc).c_str () );
  }

  //
  // RASTERIZATION
  //
  if (stats.CInvocations > 0)
  {
    sprintf ( szDesc,
                 "%s  RASTER : %5.1f%% Filled     (%s Triangles IN )\n",
                   std::string (szDesc).c_str (), 100.0 *
                       ( static_cast <double> (stats.CPrimitives) /
                         static_cast <double> (stats.CInvocations) ),
                     SK_CountToString (stats.CInvocations).c_str () );
  }

  else
  {
    sprintf ( szDesc,
                 "%s  RASTER : <Unused>\n",
                   std::string (szDesc).c_str () );
  }

  //
  // PIXEL SHADING
  //
  if (stats.PSInvocations > 0)
  {
    sprintf ( szDesc,
                 "%s  PIXEL  : %s   (%s Triangles OUT)\n",
                   std::string (szDesc).c_str (),
                     SK_CountToString (stats.PSInvocations).c_str (),
                       SK_CountToString (stats.CPrimitives).c_str () );
  }

  else
  {
    sprintf ( szDesc,
                 "%s  PIXEL  : <Unused>\n",
                   std::string (szDesc).c_str () );
  }

  //
  // COMPUTE
  //
  if (stats.CSInvocations > 0)
  {
    sprintf ( szDesc,
                 "%s  COMPUTE: %s\n",
                   std::string (szDesc).c_str (), SK_CountToString (stats.CSInvocations).c_str () );
  }

  else
  {
    sprintf ( szDesc,
                 "%s  COMPUTE: <Unused>\n",
                   std::string (szDesc).c_str () );
  }

  return szDesc;
}


#define SK_DLL_HOOK(Backend,Func)                          \
  SK_CreateDLLHook2 ( Backend,                             \
                     #Func,                                \
    static_cast_pfn <void*> (Func),                        \
    static_cast_p2p <void > (&imp_ ## Func) ); ++GL_HOOKS;

#define SK_GL_HOOK(Func) SK_DLL_HOOK(wszBackendDLL,Func)


static volatile LONG
 __SK_GL_initialized = FALSE;

DWORD
WINAPI
SK_HookGL (LPVOID)
{
  if (! config.apis.OpenGL.hook)
  {
    SK_Thread_CloseSelf ();

    return 0;
  }

  SK_TLS* pTLS =
   SK_TLS_Bottom ();

  if (! InterlockedCompareExchangeAcquire (&__SK_GL_initialized, TRUE, FALSE))
  {
    const wchar_t* wszBackendDLL (L"OpenGL32.dll");

    cs_gl_ctx =
      new SK_Thread_HybridSpinlock (512);

    if ( StrStrIW ( SK_GetDLLName ().c_str (),
                      wszBackendDLL )
       )
    {
      SK_Modules->LoadLibrary (L"gdi32.dll");
      SK_LoadRealGL ();

      wgl_get_proc_address =
        (wglGetProcAddress_pfn)SK_GetProcAddress      (local_gl, "wglGetProcAddress");
      wgl_swap_buffers =
        (wglSwapBuffers_pfn)SK_GetProcAddress         (local_gl, "wglSwapBuffers");
      wgl_make_current =
        (wglMakeCurrent_pfn)SK_GetProcAddress         (local_gl, "wglMakeCurrent");
      wgl_share_lists =
        (wglShareLists_pfn)SK_GetProcAddress          (local_gl, "wglShareLists");
      wgl_create_context =
        (wglCreateContext_pfn)SK_GetProcAddress       (local_gl, "wglCreateContext");
      wgl_choose_pixel_format =
        (wglChoosePixelFormat_pfn)SK_GetProcAddress   (local_gl, "wglChoosePixelFormat");
      wgl_set_pixel_format =
        (wglSetPixelFormat_pfn)SK_GetProcAddress      (local_gl, "wglSetPixelFormat");
      wgl_delete_context =
        (wglDeleteContext_pfn)SK_GetProcAddress       (local_gl, "wglDeleteContext");
      wgl_swap_multiple_buffers =
        (wglSwapMultipleBuffers_pfn)SK_GetProcAddress (local_gl, "wglSwapMultipleBuffers");

      pTLS->render->gl->ctx_init_thread = true;
    }

    SK_LOG0 ( (L"Additional OpenGL Initialization"), L" OpenGL32 ");
    SK_LOG0 ( (L"================================"), L" OpenGL32 ");

    if ( StrStrIW ( SK_GetDLLName ().c_str (),
                      wszBackendDLL )
       )
    {
      // Load user-defined DLLs (Plug-In)
      SK_RunLHIfBitness (64, SK_LoadPlugIns64 (), SK_LoadPlugIns32 ());
    }

    else
    {
      SK_LOG0 ( (L"Hooking OpenGL"), L" OpenGL32 " );

      SK_LoadRealGL ();

      wgl_get_proc_address =
        (wglGetProcAddress_pfn)SK_GetProcAddress       (local_gl, "wglGetProcAddress");

      SK_CreateDLLHook2 (         SK_GetModuleFullName (local_gl).c_str (),
                                 "wglSwapBuffers",
         static_cast_pfn <void*> (wglSwapBuffers),
         static_cast_p2p <void> (&wgl_swap_buffers) );

      SK_CreateDLLHook2 (         SK_GetModuleFullName (local_gl).c_str (),
                                 "wglMakeCurrent",
         static_cast_pfn <void*> (wglMakeCurrent),
         static_cast_p2p <void> (&wgl_make_current) );

      SK_CreateDLLHook2 (         SK_GetModuleFullName (local_gl).c_str (),
                                 "wglShareLists",
         static_cast_pfn <void*> (wglShareLists),
         static_cast_p2p <void> (&wgl_share_lists) );

      SK_CreateDLLHook2 (         SK_GetModuleFullName (local_gl).c_str (),
                                 "wglCreateContext",
         static_cast_pfn <void*> (wglCreateContext),
         static_cast_p2p <void> (&wgl_create_context) );

      SK_CreateDLLHook2 (         SK_GetModuleFullName (local_gl).c_str (),
                                 "wglChoosePixelFormat",
         static_cast_pfn <void*> (wglChoosePixelFormat),
         static_cast_p2p <void> (&wgl_choose_pixel_format) );

      SK_CreateDLLHook2 (         SK_GetModuleFullName (local_gl).c_str (),
                                 "wglSetPixelFormat",
         static_cast_pfn <void*> (wglSetPixelFormat),
         static_cast_p2p <void> (&wgl_set_pixel_format) );

      SK_CreateDLLHook2 (         SK_GetModuleFullName (local_gl).c_str (),
                                 "wglSwapMultipleBuffers",
         static_cast_pfn <void*> (wglSwapMultipleBuffers),
         static_cast_p2p <void> (&wgl_swap_multiple_buffers) );

      SK_CreateDLLHook2 (         SK_GetModuleFullName (local_gl).c_str (),
                                 "wglDeleteContext",
         static_cast_pfn <void*> (wglDeleteContext),
         static_cast_p2p <void> (&wgl_delete_context) );

      SK_GL_HOOK(wglGetCurrentContext);
    //SK_GL_HOOK(wglGetCurrentDC);

      pTLS->render->gl->ctx_init_thread = true;

      if (SK_GetDLLRole () == DLL_ROLE::OpenGL)
      {
        if (SK_GetFramesDrawn () > 1)
          SK_ApplyQueuedHooks ();

        // Load user-defined DLLs (Plug-In)
        SK_RunLHIfBitness (64, SK_LoadPlugIns64 (), SK_LoadPlugIns32 ());
      }

      ++GL_HOOKS;
      ++GL_HOOKS;

      SK_GL_HOOK(glAccum);
      SK_GL_HOOK(glAlphaFunc);
      SK_GL_HOOK(glAreTexturesResident);
      SK_GL_HOOK(glArrayElement);
      SK_GL_HOOK(glBegin);
      SK_GL_HOOK(glBindTexture);
      SK_GL_HOOK(glBitmap);
      SK_GL_HOOK(glBlendFunc);
      SK_GL_HOOK(glCallList);
      SK_GL_HOOK(glCallLists);
      SK_GL_HOOK(glClear);
      SK_GL_HOOK(glClearAccum);
      SK_GL_HOOK(glClearColor);
      SK_GL_HOOK(glClearDepth);
      SK_GL_HOOK(glClearIndex);
      SK_GL_HOOK(glClearStencil);
      SK_GL_HOOK(glClipPlane);
      SK_GL_HOOK(glColor3b);
      SK_GL_HOOK(glColor3bv);
      SK_GL_HOOK(glColor3d);
      SK_GL_HOOK(glColor3dv);
      SK_GL_HOOK(glColor3f);
      SK_GL_HOOK(glColor3fv);
      SK_GL_HOOK(glColor3i);
      SK_GL_HOOK(glColor3iv);
      SK_GL_HOOK(glColor3s);
      SK_GL_HOOK(glColor3sv);
      SK_GL_HOOK(glColor3ub);
      SK_GL_HOOK(glColor3ubv);
      SK_GL_HOOK(glColor3ui);
      SK_GL_HOOK(glColor3uiv);
      SK_GL_HOOK(glColor3us);
      SK_GL_HOOK(glColor3usv);
      SK_GL_HOOK(glColor4b);
      SK_GL_HOOK(glColor4bv);
      SK_GL_HOOK(glColor4d);
      SK_GL_HOOK(glColor4dv);
      SK_GL_HOOK(glColor4f);
      SK_GL_HOOK(glColor4fv);
      SK_GL_HOOK(glColor4i);
      SK_GL_HOOK(glColor4iv);
      SK_GL_HOOK(glColor4s);
      SK_GL_HOOK(glColor4sv);
      SK_GL_HOOK(glColor4ub);
      SK_GL_HOOK(glColor4ubv);
      SK_GL_HOOK(glColor4ui);
      SK_GL_HOOK(glColor4uiv);
      SK_GL_HOOK(glColor4us);
      SK_GL_HOOK(glColor4usv);
      SK_GL_HOOK(glColorMask);
      SK_GL_HOOK(glColorMaterial);
      SK_GL_HOOK(glColorPointer);
      SK_GL_HOOK(glCopyPixels);
      SK_GL_HOOK(glCopyTexImage1D);
      SK_GL_HOOK(glCopyTexImage2D);
      SK_GL_HOOK(glCopyTexSubImage1D);
      SK_GL_HOOK(glCopyTexSubImage2D);
      SK_GL_HOOK(glCullFace);
      //SK_GL_HOOK(glDebugEntry);
      SK_GL_HOOK(glDeleteLists);
      SK_GL_HOOK(glDeleteTextures);
      SK_GL_HOOK(glDepthFunc);
      SK_GL_HOOK(glDepthMask);
      SK_GL_HOOK(glDepthRange);
      SK_GL_HOOK(glDisable);
      SK_GL_HOOK(glDisableClientState);
      SK_GL_HOOK(glDrawArrays);
      SK_GL_HOOK(glDrawBuffer);
      SK_GL_HOOK(glDrawElements);
      SK_GL_HOOK(glDrawPixels);
      SK_GL_HOOK(glEdgeFlag);
      SK_GL_HOOK(glEdgeFlagPointer);
      SK_GL_HOOK(glEdgeFlagv);
      SK_GL_HOOK(glEnable);
      SK_GL_HOOK(glEnableClientState);
      SK_GL_HOOK(glEnd);
      SK_GL_HOOK(glEndList);
      SK_GL_HOOK(glEvalCoord1d);
      SK_GL_HOOK(glEvalCoord1dv);
      SK_GL_HOOK(glEvalCoord1f);
      SK_GL_HOOK(glEvalCoord1fv);
      SK_GL_HOOK(glEvalCoord2d);
      SK_GL_HOOK(glEvalCoord2dv);
      SK_GL_HOOK(glEvalCoord2f);
      SK_GL_HOOK(glEvalCoord2fv);
      SK_GL_HOOK(glEvalMesh1);
      SK_GL_HOOK(glEvalMesh2);
      SK_GL_HOOK(glEvalPoint1);
      SK_GL_HOOK(glEvalPoint2);
      SK_GL_HOOK(glFeedbackBuffer);
      SK_GL_HOOK(glFinish);
      SK_GL_HOOK(glFlush);
      SK_GL_HOOK(glFogf);
      SK_GL_HOOK(glFogfv);
      SK_GL_HOOK(glFogi);
      SK_GL_HOOK(glFogiv);
      SK_GL_HOOK(glFrontFace);
      SK_GL_HOOK(glFrustum);
      SK_GL_HOOK(glGenLists);
      SK_GL_HOOK(glGenTextures);
      SK_GL_HOOK(glGetBooleanv);
      SK_GL_HOOK(glGetClipPlane);
      SK_GL_HOOK(glGetDoublev);
      SK_GL_HOOK(glGetError);
      SK_GL_HOOK(glGetFloatv);
      SK_GL_HOOK(glGetIntegerv);
      SK_GL_HOOK(glGetLightfv);
      SK_GL_HOOK(glGetLightiv);
      SK_GL_HOOK(glGetMapdv);
      SK_GL_HOOK(glGetMapfv);
      SK_GL_HOOK(glGetMapiv);
      SK_GL_HOOK(glGetMaterialfv);
      SK_GL_HOOK(glGetMaterialiv);
      SK_GL_HOOK(glGetPixelMapfv);
      SK_GL_HOOK(glGetPixelMapuiv);
      SK_GL_HOOK(glGetPixelMapusv);
      SK_GL_HOOK(glGetPointerv);
      SK_GL_HOOK(glGetPolygonStipple);
      SK_GL_HOOK(glGetString);
      SK_GL_HOOK(glGetTexEnvfv);
      SK_GL_HOOK(glGetTexEnviv);
      SK_GL_HOOK(glGetTexGendv);
      SK_GL_HOOK(glGetTexGenfv);
      SK_GL_HOOK(glGetTexGeniv);
      SK_GL_HOOK(glGetTexImage);
      SK_GL_HOOK(glGetTexLevelParameterfv);
      SK_GL_HOOK(glGetTexLevelParameteriv);
      SK_GL_HOOK(glGetTexParameterfv);
      SK_GL_HOOK(glGetTexParameteriv);
      SK_GL_HOOK(glHint);
      SK_GL_HOOK(glIndexMask);
      SK_GL_HOOK(glIndexPointer);
      SK_GL_HOOK(glIndexd);
      SK_GL_HOOK(glIndexdv);
      SK_GL_HOOK(glIndexf);
      SK_GL_HOOK(glIndexfv);
      SK_GL_HOOK(glIndexi);
      SK_GL_HOOK(glIndexiv);
      SK_GL_HOOK(glIndexs);
      SK_GL_HOOK(glIndexsv);
      SK_GL_HOOK(glIndexub);
      SK_GL_HOOK(glIndexubv);
      SK_GL_HOOK(glInitNames);
      SK_GL_HOOK(glInterleavedArrays);
      SK_GL_HOOK(glIsEnabled);
      SK_GL_HOOK(glIsList);
      SK_GL_HOOK(glIsTexture);
      SK_GL_HOOK(glLightModelf);
      SK_GL_HOOK(glLightModelfv);
      SK_GL_HOOK(glLightModeli);
      SK_GL_HOOK(glLightModeliv);
      SK_GL_HOOK(glLightf);
      SK_GL_HOOK(glLightfv);
      SK_GL_HOOK(glLighti);
      SK_GL_HOOK(glLightiv);
      SK_GL_HOOK(glLineStipple);
      SK_GL_HOOK(glLineWidth);
      SK_GL_HOOK(glListBase);
      SK_GL_HOOK(glLoadIdentity);
      SK_GL_HOOK(glLoadMatrixd);
      SK_GL_HOOK(glLoadMatrixf);
      SK_GL_HOOK(glLoadName);
      SK_GL_HOOK(glLogicOp);
      SK_GL_HOOK(glMap1d);
      SK_GL_HOOK(glMap1f);
      SK_GL_HOOK(glMap2d);
      SK_GL_HOOK(glMap2f);
      SK_GL_HOOK(glMapGrid1d);
      SK_GL_HOOK(glMapGrid1f);
      SK_GL_HOOK(glMapGrid2d);
      SK_GL_HOOK(glMapGrid2f);
      SK_GL_HOOK(glMaterialf);
      SK_GL_HOOK(glMaterialfv);
      SK_GL_HOOK(glMateriali);
      SK_GL_HOOK(glMaterialiv);
      SK_GL_HOOK(glMatrixMode);
      SK_GL_HOOK(glMultMatrixd);
      SK_GL_HOOK(glMultMatrixf);
      SK_GL_HOOK(glNewList);
      SK_GL_HOOK(glNormal3b);
      SK_GL_HOOK(glNormal3bv);
      SK_GL_HOOK(glNormal3d);
      SK_GL_HOOK(glNormal3dv);
      SK_GL_HOOK(glNormal3f);
      SK_GL_HOOK(glNormal3fv);
      SK_GL_HOOK(glNormal3i);
      SK_GL_HOOK(glNormal3iv);
      SK_GL_HOOK(glNormal3s);
      SK_GL_HOOK(glNormal3sv);
      SK_GL_HOOK(glNormalPointer);
      SK_GL_HOOK(glOrtho);
      SK_GL_HOOK(glPassThrough);
      SK_GL_HOOK(glPixelMapfv);
      SK_GL_HOOK(glPixelMapuiv);
      SK_GL_HOOK(glPixelMapusv);
      SK_GL_HOOK(glPixelStoref);
      SK_GL_HOOK(glPixelStorei);
      SK_GL_HOOK(glPixelTransferf);
      SK_GL_HOOK(glPixelTransferi);
      SK_GL_HOOK(glPixelZoom);
      SK_GL_HOOK(glPointSize);
      SK_GL_HOOK(glPolygonMode);
      SK_GL_HOOK(glPolygonOffset);
      SK_GL_HOOK(glPolygonStipple);
      SK_GL_HOOK(glPopAttrib);
      SK_GL_HOOK(glPopClientAttrib);
      SK_GL_HOOK(glPopMatrix);
      SK_GL_HOOK(glPopName);
      SK_GL_HOOK(glPrioritizeTextures);
      SK_GL_HOOK(glPushAttrib);
      SK_GL_HOOK(glPushClientAttrib);
      SK_GL_HOOK(glPushMatrix);
      SK_GL_HOOK(glPushName);
      SK_GL_HOOK(glRasterPos2d);
      SK_GL_HOOK(glRasterPos2dv);
      SK_GL_HOOK(glRasterPos2f);
      SK_GL_HOOK(glRasterPos2fv);
      SK_GL_HOOK(glRasterPos2i);
      SK_GL_HOOK(glRasterPos2iv);
      SK_GL_HOOK(glRasterPos2s);
      SK_GL_HOOK(glRasterPos2sv);
      SK_GL_HOOK(glRasterPos3d);
      SK_GL_HOOK(glRasterPos3dv);
      SK_GL_HOOK(glRasterPos3f);
      SK_GL_HOOK(glRasterPos3fv);
      SK_GL_HOOK(glRasterPos3i);
      SK_GL_HOOK(glRasterPos3iv);
      SK_GL_HOOK(glRasterPos3s);
      SK_GL_HOOK(glRasterPos3sv);
      SK_GL_HOOK(glRasterPos4d);
      SK_GL_HOOK(glRasterPos4dv);
      SK_GL_HOOK(glRasterPos4f);
      SK_GL_HOOK(glRasterPos4fv);
      SK_GL_HOOK(glRasterPos4i);
      SK_GL_HOOK(glRasterPos4iv);
      SK_GL_HOOK(glRasterPos4s);
      SK_GL_HOOK(glRasterPos4sv);
      SK_GL_HOOK(glReadBuffer);
      SK_GL_HOOK(glReadPixels);
      SK_GL_HOOK(glRectd);
      SK_GL_HOOK(glRectdv);
      SK_GL_HOOK(glRectf);
      SK_GL_HOOK(glRectfv);
      SK_GL_HOOK(glRecti);
      SK_GL_HOOK(glRectiv);
      SK_GL_HOOK(glRects);
      SK_GL_HOOK(glRectsv);
      SK_GL_HOOK(glRenderMode);
      SK_GL_HOOK(glRotated);
      SK_GL_HOOK(glRotatef);
      SK_GL_HOOK(glScaled);
      SK_GL_HOOK(glScalef);
      SK_GL_HOOK(glScissor);
      SK_GL_HOOK(glSelectBuffer);
      SK_GL_HOOK(glShadeModel);
      SK_GL_HOOK(glStencilFunc);
      SK_GL_HOOK(glStencilMask);
      SK_GL_HOOK(glStencilOp);
      SK_GL_HOOK(glTexCoord1d);
      SK_GL_HOOK(glTexCoord1dv);
      SK_GL_HOOK(glTexCoord1f);
      SK_GL_HOOK(glTexCoord1fv);
      SK_GL_HOOK(glTexCoord1i);
      SK_GL_HOOK(glTexCoord1iv);
      SK_GL_HOOK(glTexCoord1s);
      SK_GL_HOOK(glTexCoord1sv);
      SK_GL_HOOK(glTexCoord2d);
      SK_GL_HOOK(glTexCoord2dv);
      SK_GL_HOOK(glTexCoord2f);
      SK_GL_HOOK(glTexCoord2fv);
      SK_GL_HOOK(glTexCoord2i);
      SK_GL_HOOK(glTexCoord2iv);
      SK_GL_HOOK(glTexCoord2s);
      SK_GL_HOOK(glTexCoord2sv);
      SK_GL_HOOK(glTexCoord3d);
      SK_GL_HOOK(glTexCoord3dv);
      SK_GL_HOOK(glTexCoord3f);
      SK_GL_HOOK(glTexCoord3fv);
      SK_GL_HOOK(glTexCoord3i);
      SK_GL_HOOK(glTexCoord3iv);
      SK_GL_HOOK(glTexCoord3s);
      SK_GL_HOOK(glTexCoord3sv);
      SK_GL_HOOK(glTexCoord4d);
      SK_GL_HOOK(glTexCoord4dv);
      SK_GL_HOOK(glTexCoord4f);
      SK_GL_HOOK(glTexCoord4fv);
      SK_GL_HOOK(glTexCoord4i);
      SK_GL_HOOK(glTexCoord4iv);
      SK_GL_HOOK(glTexCoord4s);
      SK_GL_HOOK(glTexCoord4sv);
      SK_GL_HOOK(glTexCoordPointer);
      SK_GL_HOOK(glTexEnvf);
      SK_GL_HOOK(glTexEnvfv);
      SK_GL_HOOK(glTexEnvi);
      SK_GL_HOOK(glTexEnviv);
      SK_GL_HOOK(glTexGend);
      SK_GL_HOOK(glTexGendv);
      SK_GL_HOOK(glTexGenf);
      SK_GL_HOOK(glTexGenfv);
      SK_GL_HOOK(glTexGeni);
      SK_GL_HOOK(glTexGeniv);
      SK_GL_HOOK(glTexImage1D);
      SK_GL_HOOK(glTexImage2D);
      SK_GL_HOOK(glTexParameterf);
      SK_GL_HOOK(glTexParameterfv);
      SK_GL_HOOK(glTexParameteri);
      SK_GL_HOOK(glTexParameteriv);
      SK_GL_HOOK(glTexSubImage1D);
      SK_GL_HOOK(glTexSubImage2D);
      SK_GL_HOOK(glTranslated);
      SK_GL_HOOK(glTranslatef);
      SK_GL_HOOK(glVertex2d);
      SK_GL_HOOK(glVertex2dv);
      SK_GL_HOOK(glVertex2f);
      SK_GL_HOOK(glVertex2fv);
      SK_GL_HOOK(glVertex2i);
      SK_GL_HOOK(glVertex2iv);
      SK_GL_HOOK(glVertex2s);
      SK_GL_HOOK(glVertex2sv);
      SK_GL_HOOK(glVertex3d);
      SK_GL_HOOK(glVertex3dv);
      SK_GL_HOOK(glVertex3f);
      SK_GL_HOOK(glVertex3fv);
      SK_GL_HOOK(glVertex3i);
      SK_GL_HOOK(glVertex3iv);
      SK_GL_HOOK(glVertex3s);
      SK_GL_HOOK(glVertex3sv);
      SK_GL_HOOK(glVertex4d);
      SK_GL_HOOK(glVertex4dv);
      SK_GL_HOOK(glVertex4f);
      SK_GL_HOOK(glVertex4fv);
      SK_GL_HOOK(glVertex4i);
      SK_GL_HOOK(glVertex4iv);
      SK_GL_HOOK(glVertex4s);
      SK_GL_HOOK(glVertex4sv);
      SK_GL_HOOK(glVertexPointer);
      SK_GL_HOOK(glViewport);

      SK_GL_HOOK(wglCopyContext);

    //SK_GL_HOOK(wglGetProcAddress);
    //SK_GL_HOOK(wglCreateContext);

      SK_GL_HOOK(wglUseFontBitmapsA);
      SK_GL_HOOK(wglUseFontBitmapsW);

      SK_GL_HOOK(wglGetPixelFormat);
    //SK_GL_HOOK(wglSetPixelFormat);
    //SK_GL_HOOK(wglChoosePixelFormat);

      SK_GL_HOOK(wglDescribePixelFormat);
    //SK_GL_HOOK(wglDescribeLayerPlane);
      SK_GL_HOOK(wglCreateLayerContext);
    //SK_GL_HOOK(wglGetLayerPaletteEntries);
    //SK_GL_HOOK(wglRealizeLayerPalette);
    //SK_GL_HOOK(wglSetLayerPaletteEntries);

      SK_LOG0 ( (L" @ %lu functions hooked",
                       GL_HOOKS ),
                 L" OpenGL32 " );
    }

    //
    // This will invoke wglSwapBuffers (...); hooking it is useful
    //   in order to control when the overlay is drawn.
    //
    SK_CreateDLLHook2 (       L"gdi32.dll",
                               "SwapBuffers",
       static_cast_pfn <void*> (SwapBuffers),
       static_cast_p2p <void > (&gdi_swap_buffers) );

    // Temporarily setup function pointers, they will be overwritten when queued
    //   hooks are installed.
    if (wgl_get_proc_address == nullptr)
        wgl_get_proc_address =
      (wglGetProcAddress_pfn)SK_GetProcAddress      (local_gl, "wglGetProcAddress");
    if (wgl_swap_buffers == nullptr)
        wgl_swap_buffers =
      (wglSwapBuffers_pfn)SK_GetProcAddress         (local_gl, "wglSwapBuffers");
    if (wgl_make_current == nullptr)
        wgl_make_current =
      (wglMakeCurrent_pfn)SK_GetProcAddress         (local_gl, "wglMakeCurrent");
    if (wgl_share_lists == nullptr)
        wgl_share_lists =
      (wglShareLists_pfn)SK_GetProcAddress          (local_gl, "wglShareLists");
    if (wgl_create_context == nullptr)
        wgl_create_context =
      (wglCreateContext_pfn)SK_GetProcAddress       (local_gl, "wglCreateContext");
    if (wgl_choose_pixel_format == nullptr)
        wgl_choose_pixel_format =
      (wglChoosePixelFormat_pfn)SK_GetProcAddress   (local_gl, "wglChoosePixelFormat");
    if (wgl_set_pixel_format == nullptr)
        wgl_set_pixel_format =
      (wglSetPixelFormat_pfn)SK_GetProcAddress      (local_gl, "wglSetPixelFormat");
    if (wgl_delete_context == nullptr)
        wgl_delete_context =
      (wglDeleteContext_pfn)SK_GetProcAddress       (local_gl, "wglDeleteContext");
    if (wgl_swap_multiple_buffers == nullptr)
        wgl_swap_multiple_buffers =
      (wglSwapMultipleBuffers_pfn)SK_GetProcAddress (local_gl, "wglSwapMultipleBuffers");

    HWND hWndDummy =
      CreateWindowW ( L"STATIC", nullptr, 0, 0, 0, 0, 0,
                        nullptr, nullptr, nullptr, 0 );

    if (hWndDummy != nullptr)
    {
      HDC hDC =
        GetDC (hWndDummy);

      if (hDC != nullptr)
      {
        PIXELFORMATDESCRIPTOR pfd =
        {             sizeof (pfd), 1,
                              PFD_DOUBLEBUFFER   |
                              PFD_DRAW_TO_WINDOW |
                              PFD_SUPPORT_OPENGL,
                              0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0
        };

        if ( SetPixelFormat       (      hDC,
               wgl_choose_pixel_format ( hDC, &pfd ),
                                              &pfd ) )
        {
          HGLRC hglrc =
            wgl_create_context (hDC);

          if (hglrc != nullptr)
          {
            wgl_make_current (hDC, hglrc);

            wgl_choose_pixel_format_arb = (wglChoosePixelFormatARB_pfn)
              wgl_get_proc_address ("wglChoosePixelFormatARB");

            if ( MH_OK == SK_CreateFuncHook ( L"wglCreateContextAttribsARB", static_cast_pfn <void *> (
                         wgl_get_proc_address ("wglCreateContextAttribsARB")),
                       static_cast_pfn <void*>( wglCreateContextAttribsARB_SK),
                       static_cast_p2p <void >(&wgl_create_context_attribs) )
               )
            {
              MH_QueueEnableHook (
                static_cast_pfn <void*> (wgl_get_proc_address ("wglCreateContextAttribsARB"))
              );
            }

            if ( MH_OK == SK_CreateFuncHook ( L"glRenderbufferStorage", static_cast_pfn <void *> (
                         wgl_get_proc_address ("glRenderbufferStorage")),
                       static_cast_pfn <void*>( glRenderbufferStorage_SK),
                       static_cast_p2p <void >(&gl_renderbuffer_storage) )
               )
            {
              MH_QueueEnableHook (
                static_cast_pfn <void*> (wgl_get_proc_address ("glRenderbufferStorage"))
              );
            }

            if ( MH_OK == SK_CreateFuncHook ( L"glFramebufferTexture", static_cast_pfn <void *> (
                         wgl_get_proc_address ("glFramebufferTexture")),
                       static_cast_pfn <void*>( glFramebufferTexture_SK),
                       static_cast_p2p <void >(&gl_framebuffer_texture) )
               )
            {
              MH_QueueEnableHook (
                static_cast_pfn <void*> (wgl_get_proc_address ("glFramebufferTexture"))
              );
            }

            if ( MH_OK == SK_CreateFuncHook ( L"glFramebufferTexture2D", static_cast_pfn <void *> (
                         wgl_get_proc_address ("glFramebufferTexture2D")),
                       static_cast_pfn <void*>( glFramebufferTexture2D_SK),
                       static_cast_p2p <void >(&gl_framebuffer_texture2d) )
               )
            {
              MH_QueueEnableHook (
                static_cast_pfn <void*> (wgl_get_proc_address ("glFramebufferTexture2D"))
              );
            }

            if ( MH_OK == SK_CreateFuncHook ( L"glNamedRenderbufferStorage", static_cast_pfn <void *> (
                         wgl_get_proc_address ("glNamedRenderbufferStorage")),
                       static_cast_pfn <void*>( glNamedRenderbufferStorage_SK),
                       static_cast_p2p <void >(&gl_named_renderbuffer_storage) )
               )
            {
              MH_QueueEnableHook (
                static_cast_pfn <void*> (wgl_get_proc_address ("glNamedRenderbufferStorage"))
              );
            }

            if ( MH_OK == SK_CreateFuncHook ( L"glNamedFramebufferTexture", static_cast_pfn <void *> (
                         wgl_get_proc_address ("glNamedFramebufferTexture")),
                       static_cast_pfn <void*>( glNamedFramebufferTexture_SK),
                       static_cast_p2p <void >(&gl_named_framebuffer_texture) )
               )
            {
              MH_QueueEnableHook (
                static_cast_pfn <void*> (wgl_get_proc_address ("glNamedFramebufferTexture"))
              );
            }

            wgl_delete_context (hglrc);
          }
        }

        ReleaseDC (hWndDummy, hDC);
      }

      DestroyWindow (hWndDummy);
    }


    pTLS->render->gl->ctx_init_thread = false;

    if (SK_GetFramesDrawn () > 1)
      SK_ApplyQueuedHooks ();

    WriteRelease                (&__gl_ready,    TRUE);
    InterlockedIncrementRelease (&__SK_GL_initialized);
  }

  SK_Thread_SpinUntilAtomicMin (&__SK_GL_initialized, 2);

  SK_Thread_CloseSelf ();

  return 0;
}


__declspec (noinline)
void
WINAPI
glRenderbufferStorage_SK ( GLenum  target,
                           GLenum  internalformat,
                           GLsizei width,
                           GLsizei height )
{
  SK_LOG0 ( ( L"[%08x (tid=%04x)]  glRenderbufferStorage "
              L"(target=%x, internalformat=%x, width=%i, height=%i)",
              SK_TLS_Bottom ()->render->gl->current_hwnd,
              SK_Thread_GetCurrentId (   ),
              target, internalformat, width, height ),
              L" OpenGL32 " );

  if (__SK_HDR_16BitSwap || config.render.gl.enable_16bit_hdr)
  {
    if (internalformat == GL_RGBA8)
    {
      internalformat = GL_RGBA16F;
    }
  }

  gl_renderbuffer_storage (target, internalformat, width, height);
}

__declspec (noinline)
void
WINAPI
glNamedRenderbufferStorage_SK ( GLuint  renderbuffer,
                                GLenum  internalformat,
                                GLsizei width,
                                GLsizei height )
{
  SK_LOG0 ( ( L"[%08x (tid=%04x)]  glNamedRenderbufferStorage "
              L"(renderbuffer=%ui, internalformat=%x, width=%i, height=%i)",
              SK_TLS_Bottom ()->render->gl->current_hwnd,
              SK_Thread_GetCurrentId (   ),
              renderbuffer, internalformat, width, height ),
              L" OpenGL32 " );

  if (__SK_HDR_16BitSwap || config.render.gl.enable_16bit_hdr)
  {
    if (internalformat == GL_RGBA8)
    {
      internalformat = GL_RGBA16F;
    }
  }

  gl_named_renderbuffer_storage (renderbuffer, internalformat, width, height);
}

__declspec (noinline)
void
WINAPI
glFramebufferTexture_SK ( GLenum target,
                          GLenum attachment,
                          GLuint texture,
                          GLint  level )
{
  SK_LOG_FIRST_CALL

  if (__SK_HDR_16BitSwap || config.render.gl.enable_16bit_hdr)
  {
    SK_LOGi1 (L"glFramebufferTexture (...)");
  }

  gl_framebuffer_texture (target, attachment, texture, level);
}

#include <SpecialK/render/dxgi/dxgi_hdr.h>

__declspec (noinline)
void
WINAPI
glNamedFramebufferTexture_SK ( GLuint framebuffer,
                               GLenum attachment,
                               GLuint texture,
                               GLint  level )
{
  SK_LOG_FIRST_CALL

  if ( __SK_HDR_16BitSwap || config.render.gl.enable_16bit_hdr )
  {
    SK_LOGi1 (L"glNamedFramebufferTexture (...)");

    static GLuint                                      maxAttachments = 0;
    glGetIntegerv (GL_MAX_COLOR_ATTACHMENTS, (GLint *)&maxAttachments);

    if (attachment >= GL_COLOR_ATTACHMENT0 && attachment < GL_COLOR_ATTACHMENT0 + maxAttachments && level == 0)
    {
      GLint                                  boundTex2D = 0;
      glGetIntegerv (GL_TEXTURE_BINDING_2D, &boundTex2D);

      GLint                                    textureFmt = 0;
      glBindTexture            (GL_TEXTURE_2D, texture);
      glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &textureFmt);

      GLint immutable = GL_FALSE;

      if (__glewGetTextureParameterIivEXT != nullptr)
              glGetTextureParameterIivEXT (texture, GL_TEXTURE_2D, GL_TEXTURE_IMMUTABLE_FORMAT, &immutable);

      GLint                                                           texWidth, texHeight;
      glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,  &texWidth);
      glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texHeight);

      glGetError ();

      if (immutable == GL_FALSE)
      {
        bool rgb =
          (  textureFmt == GL_RGB   || textureFmt == GL_RGB8 );
        if ( textureFmt == GL_RGBA8 || textureFmt == GL_RGB8 ||
             textureFmt == GL_RGBA  || textureFmt == GL_RGB )
        {
          InterlockedIncrement (&SK_HDR_RenderTargets_8bpc->CandidatesSeen);

          if (SK_HDR_RenderTargets_8bpc->PromoteTo16Bit)
          {
           //glBindTexture    (GL_TEXTURE_2D,       0);
           //glDeleteTextures (1,            &texture);
           //glBindTexture    (GL_TEXTURE_2D, texture);
           //glTexStorage2D   (GL_TEXTURE_2D, 1, rgb ?
           //                              GL_RGB16F : GL_RGBA16F, texWidth, texHeight);

            glTexImage2D (GL_TEXTURE_2D, 0, rgb ? GL_RGB16F : GL_RGBA16F, texWidth, texHeight, 0,
                                            rgb ? GL_RGB    : GL_RGBA, GL_FLOAT, nullptr);

            if (glGetError () == GL_NO_ERROR)
            {
              InterlockedAdd64     (&SK_HDR_RenderTargets_8bpc->BytesAllocated, 4LL * texWidth * texHeight);
              InterlockedIncrement (&SK_HDR_RenderTargets_8bpc->TargetsUpgraded);

              SK_LOGi1 (L"GL_RGB{A}[8] replaced with GL_RGB{A}16F");
            }
          }
        }

        else if ( textureFmt == GL_R11F_G11F_B10F ||
                  textureFmt == GL_RGB10_A2 )
        {
          if (textureFmt == GL_R11F_G11F_B10F)
          {
            InterlockedIncrement (&SK_HDR_RenderTargets_11bpc->CandidatesSeen);

            if (SK_HDR_RenderTargets_11bpc->PromoteTo16Bit)
            {
             //glBindTexture    (GL_TEXTURE_2D,       0);
             //glDeleteTextures (1,            &texture);
             //glBindTexture    (GL_TEXTURE_2D, texture);
             //glTexStorage2D   (GL_TEXTURE_2D, 1, GL_RGB16F, texWidth, texHeight);
              glTexImage2D     (GL_TEXTURE_2D, 0, GL_RGB16F, texWidth, texHeight, 0,
                                                  GL_RGB,    GL_FLOAT, nullptr);

              if (glGetError () == GL_NO_ERROR)
              {
                InterlockedAdd64     (&SK_HDR_RenderTargets_11bpc->BytesAllocated, 4LL * texWidth * texHeight);
                InterlockedIncrement (&SK_HDR_RenderTargets_11bpc->TargetsUpgraded);

                SK_LOGi1 (L"GL_R11F_G11F_B10F replaced with GL_RGB{A}16F");
              }
            }
          }

          else if (textureFmt == GL_RGB10_A2)
          {
            InterlockedIncrement (&SK_HDR_RenderTargets_10bpc->CandidatesSeen);

            if (SK_HDR_RenderTargets_10bpc->PromoteTo16Bit)
            {
            //glBindTexture    (GL_TEXTURE_2D,       0);
            //glDeleteTextures (1,            &texture);
            //glBindTexture    (GL_TEXTURE_2D, texture);
            //glTexStorage2D   (GL_TEXTURE_2D, 1, GL_RGBA16F, texWidth, texHeight);
              glTexImage2D     (GL_TEXTURE_2D, 0, GL_RGBA16F, texWidth, texHeight, 0,
                                                  GL_RGBA,    GL_FLOAT, nullptr);

              if (glGetError () == GL_NO_ERROR)
              {
                InterlockedAdd64     (&SK_HDR_RenderTargets_10bpc->BytesAllocated, 4LL * texWidth * texHeight);
                InterlockedIncrement (&SK_HDR_RenderTargets_10bpc->TargetsUpgraded);

                SK_LOGi1 (L"GL_RGB10_A2 replaced with GL_RGBA16F");
              }
            }
          }
        }

        glBindTexture (GL_TEXTURE_2D, boundTex2D);
      }

      else SK_LOGi1 (L" -> Format: %x", textureFmt);
    }
  }

  gl_named_framebuffer_texture (framebuffer, attachment, texture, level);
}


__declspec (noinline)
void
WINAPI
glFramebufferTexture2D_SK ( GLenum target,
                            GLenum attachment,
                            GLenum textarget,
                            GLuint texture,
                            GLint  level )
{
  SK_LOG_FIRST_CALL

  if (__SK_HDR_16BitSwap || config.render.gl.enable_16bit_hdr)
  {
    SK_LOGi1 (L"glFramebufferTexture2D (...)");

    if (textarget == GL_TEXTURE_2D)
    {
      static GLuint                                      maxAttachments = 0;
      glGetIntegerv (GL_MAX_COLOR_ATTACHMENTS, (GLint *)&maxAttachments);

      if (attachment >= GL_COLOR_ATTACHMENT0 && attachment < GL_COLOR_ATTACHMENT0 + maxAttachments)
      {
        GLint                                  boundTex2D = 0;
        glGetIntegerv (GL_TEXTURE_BINDING_2D, &boundTex2D);

        GLint                                    textureFmt = 0;
        glBindTexture            (GL_TEXTURE_2D, texture);
        glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &textureFmt);

        SK_LOGi1 (L" -> Format: %x", textureFmt);

        bool rgb =
          (  textureFmt == GL_RGB   || textureFmt == GL_RGB8 );
        if ( textureFmt == GL_RGBA8 || textureFmt == GL_RGB8 ||
             textureFmt == GL_RGBA  || textureFmt == GL_RGB )
        {
          if (__glewGetTextureParameterIivEXT != nullptr)
          {
            GLint                                                                              immutable = GL_FALSE;
            glGetTextureParameterIivEXT (texture, GL_TEXTURE_2D, GL_TEXTURE_IMMUTABLE_FORMAT, &immutable);

            if (immutable == GL_FALSE)
            {
              GLint                                                           texWidth, texHeight;
              glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,  &texWidth);
              glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texHeight);

              SK_LOGi1 (L"GL_RGB{A}[8] replaced with GL_RGB{A}16F");

              glTexImage2D (GL_TEXTURE_2D, 0, rgb ? GL_RGB16F : GL_RGBA16F, texWidth, texHeight, 0,
                                              rgb ? GL_RGB    : GL_RGBA, GL_FLOAT, nullptr);
            }
          }
        }

        glBindTexture (GL_TEXTURE_2D, boundTex2D);
      }
    }
  }

#if 0
  if (__SK_HDR_10BitSwap || config.render.gl.enable_10bit_hdr)
  {
    SK_LOGi1 (L"glFramebufferTexture2D (...)");

    if (textarget == GL_TEXTURE_2D)
    {
      static GLuint                                      maxAttachments = 0;
      glGetIntegerv (GL_MAX_COLOR_ATTACHMENTS, (GLint *)&maxAttachments);

      if (attachment >= GL_COLOR_ATTACHMENT0 && attachment < GL_COLOR_ATTACHMENT0 + maxAttachments)
      {
        GLint                                  boundTex2D = 0;
        glGetIntegerv (GL_TEXTURE_BINDING_2D, &boundTex2D);

        GLint                                    textureFmt = 0;
        glBindTexture            (GL_TEXTURE_2D, texture);
        glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &textureFmt);

        SK_LOGi1 (L" -> Format: %x", textureFmt);

        bool rgb =
          (  textureFmt == GL_RGB   || textureFmt == GL_RGB8 );
        if ( textureFmt == GL_RGBA8 || textureFmt == GL_RGB8 ||
             textureFmt == GL_RGBA  || textureFmt == GL_RGB )
        {
          GLint immutable = GL_FALSE;

          if (__glewGetTextureParameterIivEXT != nullptr)
                  glGetTextureParameterIivEXT (texture, GL_TEXTURE_2D, GL_TEXTURE_IMMUTABLE_FORMAT, &immutable);

          if (immutable == GL_FALSE)
          {
            GLint                                                           texWidth, texHeight;
            glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,  &texWidth);
            glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texHeight);

            SK_LOGi1 (L"GL_RGB{A}[8] replaced with GL_RGB10{A}[2]");

            glTexImage2D (GL_TEXTURE_2D, 0, rgb ? GL_RGB10 : GL_RGB10_A2, texWidth, texHeight, 0,
                                            rgb ? GL_RGB   : GL_RGBA, GL_UNSIGNED_INT, nullptr);
          }
        }

        glBindTexture (GL_TEXTURE_2D, boundTex2D);
      }
    }
  }
#endif

  gl_framebuffer_texture2d (target, attachment, textarget, texture, level);
}

__declspec (noinline)
int
WINAPI
wglChoosePixelFormat ( HDC                     hDC,
                 const PIXELFORMATDESCRIPTOR* ppfd )
{
  WaitForInit_GL ();

  SK_LOG0 ( ( L"[%08x (tid=%04x)]  wglChoosePixelFormat "
              L"(hDC=%x)",
              SK_TLS_Bottom ()->render->gl->current_hwnd,
              SK_Thread_GetCurrentId (   ), hDC ),
              L" OpenGL32 " );

  int ret = 0;

  const bool bOverridePixelFormat =
    config.render.gl.upgrade_zbuffer ||
    config.render.gl.prefer_10bpc    ||
    config.render.gl.enable_10bit_hdr||
    config.render.gl.enable_16bit_hdr||
    __SK_HDR_10BitSwap               ||
    __SK_HDR_16BitSwap;

  if (bOverridePixelFormat)
  {
    const bool bUse16bpc = (__SK_HDR_16BitSwap || config.render.gl.enable_16bit_hdr);
    const bool bUse10bpc = (__SK_HDR_10BitSwap || config.render.gl.enable_10bit_hdr)
                                               || config.render.gl.prefer_10bpc;

    const bool bUpgradeColor = bUse10bpc || bUse16bpc;

    const BYTE cColorBitsTotal = bUpgradeColor ? (bUse16bpc ? 48 : 30) : ppfd->cColorBits;
    const BYTE cColorBitsRed   = bUpgradeColor ? (bUse16bpc ? 16 : 10) : ppfd->cRedBits;
    const BYTE cColorBitsGreen = bUpgradeColor ? (bUse16bpc ? 16 : 10) : ppfd->cGreenBits;
    const BYTE cColorBitsBlue  = bUpgradeColor ? (bUse16bpc ? 16 : 10) : ppfd->cBlueBits;
    const BYTE cAlphaBits      = bUpgradeColor ? (bUse16bpc ? 16 :  2) : ppfd->cAlphaBits;
    const BYTE cRedShift       = bUpgradeColor ? (bUse16bpc ? 0  :  0) : ppfd->cRedShift;
    const BYTE cGreenShift     = bUpgradeColor ? (bUse16bpc ? 0  : 10) : ppfd->cGreenShift;
    const BYTE cBlueShift      = bUpgradeColor ? (bUse16bpc ? 0  : 20) : ppfd->cBlueShift;
    const BYTE cAlphaShift     = bUpgradeColor ? (bUse16bpc ? 0  : 30) : ppfd->cAlphaShift;

    // Don't even bother trying to support color index stuff
    SK_ReleaseAssert (bUpgradeColor || ppfd->iPixelType == PFD_TYPE_RGBA);

    const BYTE cDepthBits   = config.render.gl.upgrade_zbuffer ? 24 : ppfd->cDepthBits;
    const BYTE cStencilBits = config.render.gl.upgrade_zbuffer ? (ppfd->cStencilBits == 0 ? 0 : ppfd->cStencilBits <= 8 ? 8 : ppfd->cStencilBits)
                                                               :  ppfd->cStencilBits;

    PIXELFORMATDESCRIPTOR pfd =
                        *ppfd;

    pfd.nVersion     = 1;
    pfd.nSize        = sizeof (pfd);
    pfd.dwFlags      = PFD_DOUBLEBUFFER  | PFD_DRAW_TO_WINDOW |
                       PFD_SWAP_EXCHANGE | PFD_SUPPORT_COMPOSITION;
    pfd.cColorBits   = cColorBitsTotal;
    pfd.cRedBits     = cColorBitsRed;
    pfd.cGreenBits   = cColorBitsGreen;
    pfd.cBlueBits    = cColorBitsBlue;
    pfd.cAlphaBits   = cAlphaBits;
    pfd.cAlphaShift  = cAlphaShift;
    pfd.cRedShift    = cRedShift;
    pfd.cGreenShift  = cGreenShift;
    pfd.cBlueShift   = cBlueShift;
    pfd.cStencilBits = cStencilBits;
    pfd.cDepthBits   = cDepthBits;

    ret =
      wgl_choose_pixel_format (hDC, &pfd);

    if (ret != 0)
      return ret;
  }

#if 0
  PIXELFORMATDESCRIPTOR pfd_test = { };
                        pfd_test.nVersion = 1;
                        pfd_test.nSize    = sizeof (PIXELFORMATDESCRIPTOR);

  int idx = 1;

  while (0 != DescribePixelFormat (hDC, idx++, sizeof (PIXELFORMATDESCRIPTOR), &pfd_test))
  {
    if (pfd_test.cRedBits == 10 && pfd_test.cGreenBits == 10 && pfd_test.cBlueBits == 10 && pfd_test.cAlphaBits == 2)
    {
      SK_LOGi0 (
        L"RGB10A2 Format Found - idx=%d, RedShift=%d, GreenShift=%d, BlueShift=%d, AlphaShift=%d, Flags=%x, DS=%d:%d",
          idx-1, pfd_test.cRedShift, pfd_test.cGreenShift, pfd_test.cBlueShift, pfd_test.cAlphaShift, pfd_test.dwFlags,
                 pfd_test.cDepthBits, pfd_test.cStencilBits
      );
    }

    ZeroMemory (&pfd_test, sizeof (PIXELFORMATDESCRIPTOR));

    pfd_test.nVersion = 1;
    pfd_test.nSize    = sizeof (PIXELFORMATDESCRIPTOR);
  }
#endif

  ret =
    wgl_choose_pixel_format (hDC, ppfd);

  return ret;
}

__declspec (noinline)
BOOL
WINAPI
wglSetPixelFormat ( HDC                    hDC,
                    int                    iPixelFormat,
              const PIXELFORMATDESCRIPTOR *ppfd )
{
  WaitForInit_GL ();

  SK_LOG0 ( ( L"[%08x (tid=%04x)]  wglSetPixelFormat "
              L"(hDC=%x, iPixelFormat=%i)\t[%ws]",
              SK_TLS_Bottom ()->render->gl->current_hwnd,
              SK_Thread_GetCurrentId (   ), hDC, iPixelFormat, SK_GetCallerName ().c_str () ),
              L" OpenGL32 " );

  int iPixelFormatOverride = iPixelFormat;

  SK_LOG0 ( ( L" @ Requested:    "
                  L"R%" _L(PRIi8) L"G%" _L(PRIi8) L"B%"         _L(PRIi8)
                  L"%ws Depth=%"        _L(PRIi8) L" Stencil=%" _L(PRIi8),
              ppfd->cRedBits,  ppfd->cGreenBits,
              ppfd->cBlueBits, ppfd->cAlphaBits == 0 ?
                                  L" "               :
                SK_FormatStringW (L"A%" _L(PRIi8),
                               ppfd->cAlphaBits).c_str (),
                               ppfd->cDepthBits,
                               ppfd->cStencilBits ),
              L" OpenGL32 " );

  PIXELFORMATDESCRIPTOR pfd_copy = *ppfd;

  const bool bOverridePixelFormat =
    config.render.gl.upgrade_zbuffer ||
    config.render.gl.prefer_10bpc    ||
    config.render.gl.enable_10bit_hdr||
    config.render.gl.enable_16bit_hdr||
    __SK_HDR_10BitSwap               ||
    __SK_HDR_16BitSwap;

  if (bOverridePixelFormat)
  {
    const bool bUse16bpc = (__SK_HDR_16BitSwap || config.render.gl.enable_16bit_hdr);
    const bool bUse10bpc = (__SK_HDR_10BitSwap || config.render.gl.enable_10bit_hdr)
                                               || config.render.gl.prefer_10bpc;

    const bool bUpgradeColor   = bUse10bpc ||
                                 bUse16bpc;
    const int  iColorBitsTotal = bUpgradeColor ? (bUse16bpc ? 48 : 30) : ppfd->cColorBits;
    const int  iColorBitsRed   = bUpgradeColor ? (bUse16bpc ? 16 : 10) : ppfd->cRedBits;
    const int  iColorBitsGreen = bUpgradeColor ? (bUse16bpc ? 16 : 10) : ppfd->cGreenBits;
    const int  iColorBitsBlue  = bUpgradeColor ? (bUse16bpc ? 16 : 10) : ppfd->cBlueBits;
    const int  iAlphaBits      = bUpgradeColor ? (bUse16bpc ? 16 :  2) : ppfd->cAlphaBits;
    const int  iRedShift       = bUpgradeColor ? (bUse16bpc ? 0  :  0) : ppfd->cRedShift;
    const int  iGreenShift     = bUpgradeColor ? (bUse16bpc ? 0  : 10) : ppfd->cGreenShift;
    const int  iBlueShift      = bUpgradeColor ? (bUse16bpc ? 0  : 20) : ppfd->cBlueShift;
    const int  iAlphaShift     = bUpgradeColor ? (bUse16bpc ? 0  : 30) : ppfd->cAlphaShift;

    if (wgl_choose_pixel_format_arb != nullptr)
    {
      std::vector <int> attribs {
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
        WGL_SWAP_METHOD_ARB,    WGL_SWAP_EXCHANGE_ARB,
        WGL_ACCELERATION_ARB,   WGL_FULL_ACCELERATION_ARB
      };

      attribs.push_back (WGL_COLOR_BITS_ARB);
      attribs.push_back (iColorBitsTotal);
      attribs.push_back (WGL_RED_BITS_ARB);
      attribs.push_back (iColorBitsRed);
      attribs.push_back (WGL_GREEN_BITS_ARB);
      attribs.push_back (iColorBitsGreen);
      attribs.push_back (WGL_BLUE_BITS_ARB);
      attribs.push_back (iColorBitsBlue);
      attribs.push_back (WGL_ALPHA_BITS_ARB);
      attribs.push_back (iAlphaBits);

      attribs.push_back (            WGL_PIXEL_TYPE_ARB);
      attribs.push_back (bUse16bpc ? WGL_TYPE_RGBA_FLOAT_ARB
                                   : WGL_TYPE_RGBA_ARB);

      // Don't even bother trying to support color index stuff
      SK_ReleaseAssert (bUpgradeColor || ppfd->iPixelType == PFD_TYPE_RGBA);

      const int iDepthBits   = config.render.gl.upgrade_zbuffer ? 24 : ppfd->cDepthBits;
      const int iStencilBits = config.render.gl.upgrade_zbuffer ? (ppfd->cStencilBits == 0 ? 0 : ppfd->cStencilBits <= 8 ? 8 : ppfd->cStencilBits)
                                                                :  ppfd->cStencilBits;

      attribs.push_back (WGL_DEPTH_BITS_ARB);
      attribs.push_back (iDepthBits);

      attribs.push_back (WGL_STENCIL_BITS_ARB);
      attribs.push_back (iStencilBits);

      attribs.push_back (0);
      attribs.push_back (0);

      int pixelFormat = 0;
      UINT numFormats = 0;

      if (wgl_choose_pixel_format_arb (hDC, attribs.data (), nullptr, 1, &pixelFormat, &numFormats) && numFormats >= 1)
      {
        PIXELFORMATDESCRIPTOR new_pfd = *ppfd;

        DescribePixelFormat      (hDC, pixelFormat, sizeof (PIXELFORMATDESCRIPTOR), &new_pfd);
        if (wgl_set_pixel_format (hDC, pixelFormat,                                 &new_pfd))
        {
          SK_LOG0 ( ( L" * %wcDR Override: "
                          L"R%" _L(PRIi8) L"G%" _L(PRIi8) L"B%"         _L(PRIi8)
                          L"%ws Depth=%"        _L(PRIi8) L" Stencil=%" _L(PRIi8), (__SK_HDR_16BitSwap || __SK_HDR_10BitSwap) ? L'H' : L'S',
                       new_pfd.cRedBits,  new_pfd.cGreenBits,
                       new_pfd.cBlueBits, new_pfd.cAlphaBits == 0 ?
                                          L" "                    :
                        SK_FormatStringW (L"A%" _L(PRIi8),
                                          new_pfd.cAlphaBits).c_str (),
                                          new_pfd.cDepthBits,
                                          new_pfd.cStencilBits ),
                      L" OpenGL32 " );

          return TRUE;
        }
      }
    }

    pfd_copy.nVersion    = 1;
    pfd_copy.nSize       = sizeof (pfd_copy);
    pfd_copy.dwFlags     = PFD_DOUBLEBUFFER  | PFD_DRAW_TO_WINDOW |
                           PFD_SWAP_EXCHANGE | PFD_SUPPORT_COMPOSITION;
    pfd_copy.cColorBits  = static_cast <BYTE> (iColorBitsTotal);
    pfd_copy.cRedBits    = static_cast <BYTE> (iColorBitsRed);
    pfd_copy.cGreenBits  = static_cast <BYTE> (iColorBitsGreen);
    pfd_copy.cBlueBits   = static_cast <BYTE> (iColorBitsBlue);
    pfd_copy.cAlphaBits  = static_cast <BYTE> (iAlphaBits);
    pfd_copy.cAlphaShift = static_cast <BYTE> (iAlphaShift);
    pfd_copy.cRedShift   = static_cast <BYTE> (iRedShift);
    pfd_copy.cGreenShift = static_cast <BYTE> (iGreenShift);
    pfd_copy.cBlueShift  = static_cast <BYTE> (iBlueShift);

    pfd_copy.cStencilBits = config.render.gl.upgrade_zbuffer ? (ppfd->cStencilBits == 0 ? 0 : ppfd->cStencilBits <= 8 ? 8 : ppfd->cStencilBits)
                                                                  : ppfd->cStencilBits;
    pfd_copy.cDepthBits   = config.render.gl.upgrade_zbuffer ? 24 : ppfd->cDepthBits;
  }

  iPixelFormatOverride =
    wgl_choose_pixel_format (hDC, &pfd_copy);

  BOOL ret =
    wgl_set_pixel_format (hDC, iPixelFormatOverride, &pfd_copy);

  // Fallback to original values if we failed...
  if (! ret)
    wgl_set_pixel_format (hDC, iPixelFormat, ppfd);

  else
  {
    DescribePixelFormat (hDC, iPixelFormatOverride, sizeof (PIXELFORMATDESCRIPTOR), &pfd_copy);

    SK_LOG0 ( ( L" @ SK Overrides: "
                  L"R%" _L(PRIi8) L"G%" _L(PRIi8) L"B%"         _L(PRIi8)
                  L"%ws Depth=%"        _L(PRIi8) L" Stencil=%" _L(PRIi8),
              pfd_copy.cRedBits,  pfd_copy.cGreenBits,
              pfd_copy.cBlueBits, pfd_copy.cAlphaBits == 0 ?
                                  L" "               :
                SK_FormatStringW (L"A%" _L(PRIi8),
                               pfd_copy.cAlphaBits).c_str (),
                               pfd_copy.cDepthBits,
                               pfd_copy.cStencilBits ),
              L" OpenGL32 " );
  }

  return ret;
}

__declspec (noinline)
HGLRC
WINAPI
wglCreateContext (HDC hDC)
{
  WaitForInit_GL ();

  SK_LOG0 ( ( L"[%08x (tid=%04x)]  wglCreateContext "
              L"(hDC=%x)\t[%ws]",
              SK_TLS_Bottom ()->render->gl->current_hwnd,
              SK_Thread_GetCurrentId (   ), hDC, SK_GetCallerName ().c_str () ),
              L" OpenGL32 " );

  HGLRC  hglrc = nullptr;

  if (wgl_create_context_attribs != nullptr) {
    static const int                                debug_attribs [] = { WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
                                                                         WGL_CONTEXT_MINOR_VERSION_ARB, 2,
                                                                         WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
                                                                         WGL_CONTEXT_FLAGS_ARB,         WGL_CONTEXT_DEBUG_BIT_ARB,
                                                                         0, 0 };
    static const int                                default_attribs [] = { WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
                                                                           WGL_CONTEXT_MINOR_VERSION_ARB, 2,
                                                                           WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
                                                                           0, 0 };
         hglrc = wgl_create_context_attribs ( hDC, 0, config.render.gl.debug.enable ? debug_attribs : default_attribs );
  } else hglrc =         wgl_create_context ( hDC );

  if (config.render.gl.debug.enable && glDebugMessageCallbackARB != nullptr)
  {
    HGLRC hglrc_current = wglGetCurrentContext ();
    HDC   hdc_current   = wglGetCurrentDC      ();

    wglMakeCurrent (hDC, hglrc);
    {
      if (config.render.gl.debug.break_on_error)
        glEnable                (GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
      glDebugMessageCallbackARB (SK_GL_InteropDebugOutput,   hglrc);
    }
    wglMakeCurrent (hdc_current, hglrc_current);
  }

  ++SK_GL_ContextCount;

  return hglrc;
}

__declspec (noinline)
BOOL
WINAPI
wglShareLists (HGLRC ctx0, HGLRC ctx1)
{
  WaitForInit_GL ();

  SK_LOG0 ( ( L"[%08x (tid=%04x)]  wglShareLists "
              L"(ctx0=%x, ctx1=%x)\t[%ws]",
                SK_TLS_Bottom ()->render->gl->current_hwnd,
                SK_Thread_GetCurrentId (   ),
                                       ctx0, ctx1, SK_GetCallerName ().c_str () ),
              L" OpenGL32 " );

  BOOL ret =
    wgl_share_lists (ctx0, ctx1);

  if (ret != FALSE)
  {
    if (__gl_primary_context == nullptr)
        __gl_primary_context = ctx0;

    std::scoped_lock <SK_Thread_CriticalSection>
           auto_lock (*cs_gl_ctx);

    // If sharing with a shared context, then follow the shared context
    //   back to its parent
    if (__gl_shared_contexts->count (ctx0))
      __gl_shared_contexts.get ()[ctx1] = __gl_shared_contexts.get ()[ctx0];
    else
      __gl_shared_contexts.get ()[ctx1] = ctx0;
  }

  return ret;
}


BOOL
WINAPI
SK_GL_SwapInterval (int interval)
{
  if (wgl_swap_interval == nullptr && wgl_get_proc_address != nullptr)
      wgl_swap_interval =     (wglSwapIntervalEXT_pfn)
        wgl_get_proc_address ("wglSwapIntervalEXT");

  if (     wgl_swap_interval != nullptr)
    return wgl_swap_interval (interval);

  SK_ReleaseAssert (! L"No wglGetSwapIntervalEXT");

  return FALSE;
}

int
WINAPI
SK_GL_GetSwapInterval (void)
{
  if (wgl_get_swap_interval == nullptr && wgl_get_proc_address != nullptr)
      wgl_get_swap_interval = (wglGetSwapIntervalEXT_pfn)
        wgl_get_proc_address ("wglGetSwapIntervalEXT");

  if (     wgl_get_swap_interval != nullptr)
    return wgl_get_swap_interval ();

  SK_ReleaseAssert (! L"No wglGetSwapIntervalEXT");

  return 1;
}

HGLRC
WINAPI
SK_GL_GetCurrentContext (void)
{
  HGLRC hglrc = nullptr;

  if (      imp_wglGetCurrentContext != nullptr)
    hglrc = imp_wglGetCurrentContext ();

  SK_TLS* pTLS = SK_TLS_Bottom ();

  if (pTLS != nullptr)
      pTLS->render->gl->current_hglrc = hglrc;

  return hglrc;
}

 using wglGetCurrentDC_pfn = HDC (WINAPI *)(void);
static wglGetCurrentDC_pfn
__imp__wglGetCurrentDC     = nullptr;

HDC
WINAPI
SK_GL_GetCurrentDC (void)
{
  HDC hdc = nullptr;

  if (__imp__wglGetCurrentDC == nullptr)
      __imp__wglGetCurrentDC =
            (wglGetCurrentDC_pfn)SK_GetProcAddress (local_gl,
            "wglGetCurrentDC");

  if (    __imp__wglGetCurrentDC != nullptr)
    hdc = __imp__wglGetCurrentDC ();

  //HWND hwnd =
  //  WindowFromDC (hdc);

  SK_TLS* pTLS =
        SK_TLS_Bottom ();

  if (pTLS != nullptr)
  {
  //pTLS->render->gl->current_hwnd = hwnd;
    pTLS->render->gl->current_hdc  = hdc;
  }

  return hdc;
}


BOOL
SK_GL_InsertDuplicateFrame (void)
{
  if (glBlitFramebuffer == nullptr)
    return FALSE;

  RECT                                                        rect = { };
  GetClientRect (SK_TLS_Bottom ()->render->gl->current_hwnd, &rect);

  SK_GL_GhettoStateBlock_Capture ();

  glBindFramebuffer (GL_FRAMEBUFFER, 0);
  glDisable         (GL_FRAMEBUFFER_SRGB);

  glColorMask       (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

  glReadBuffer      (GL_FRONT_LEFT);
  glDrawBuffer      (GL_BACK_LEFT);

  glBlitFramebuffer ( static_cast <GLint> (rect.left),  static_cast <GLint> (rect.top),
                      static_cast <GLint> (rect.right), static_cast <GLint> (rect.bottom),
                      static_cast <GLint> (rect.left),  static_cast <GLint> (rect.top),
                      static_cast <GLint> (rect.right), static_cast <GLint> (rect.bottom),
                      GL_COLOR_BUFFER_BIT,
                      GL_NEAREST );

  SK_GL_GhettoStateBlock_Apply ();

  return TRUE;
}
/*
  Frame 0 Frame 1  ... N
        |       |      |
   90Hz |  90Hz |      |
120Hz |[60FPS]| |      |
    | |-------| |      |
|###|=|*|###|=|*|###|=*|
        |       |      |
     60Hz    60Hz   60Hz
        |       |      |
        |       |      |
  Frame 0 Frame 1  ... N

75% Input / 25% Display  =  90 FPS Latency  @  60 FPS Display Rate
*/