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

#define WIN32_LEAN_AND_MEAN
#define NOGDI

#include <SpecialK/stdafx.h>
#include <Shlwapi.h>
#include <Windows.h>

#include <SpecialK/render_backend.h>
#include <SpecialK/opengl_backend.h>
#include <SpecialK/dxgi_backend.h>

#include <SpecialK/window.h>

#include <SpecialK/log.h>
#include <SpecialK/import.h>
#include <SpecialK/utility.h>
#include <SpecialK/framerate.h>
#include <SpecialK/diagnostics/compatibility.h>

extern HMODULE WINAPI SK_GetDLL (void);

#include <SpecialK/hooks.h>
#include <SpecialK/core.h>
#include <SpecialK/config.h>

#include <imgui/backends/imgui_gl3.h>


//SK_OpenGL_KnownPrograms SK_GL_Programs;
//SK_OpenGL_KnownTextures SK_GL_Textures;
//SK_OpenGL_KnownBuffers  SK_GL_Buffers;


volatile LONG __gl_ready = FALSE;

void
WaitForInit_GL (void)
{
  while (! ReadAcquire (&__gl_ready))
    MsgWaitForMultipleObjectsEx (0, nullptr, 100, QS_ALLINPUT, MWMO_ALERTABLE);

  WaitForInit ();
}

extern bool SK_InitCOM (void);
extern HWND hWndRender;

void __stdcall
SK_GL_UpdateRenderStats (void);

extern "C++" int SK_Steam_DrawOSD (void);

typedef BOOL (WINAPI *SwapBuffers_pfn)(HDC);
typedef BOOL (WINAPI *wglSwapBuffers_pfn)(HDC);

SwapBuffers_pfn    gdi_swap_buffers = nullptr;
wglSwapBuffers_pfn wgl_swap_buffers = nullptr;

// True if this is an injected DLL rather than OpenGL32.dll
BOOL  GL_HOOKED = FALSE;
ULONG GL_HOOKS  = 0UL;

void
WINAPI
SK_HookGL (void);

extern DWORD SK_ImGui_DrawFrame (DWORD dwFlags, void* user);

unsigned int
WINAPI
DXGI_Thread (LPVOID user)
{
  UNREFERENCED_PARAMETER (user);

  if (SUCCEEDED (CoInitializeEx ( nullptr, COINIT_MULTITHREADED )))
  {
    SK::DXGI::StartBudgetThread_NoAdapter ();

    CoUninitialize ();
  }

  return 0;
}

#include <SpecialK/osd/text.h>
#include <SpecialK/osd/popup.h>

#ifndef SK_BUILD__INSTALLER
#include <CEGUI/RendererModules/OpenGL/GL3Renderer.h>

CEGUI::OpenGL3Renderer* cegGL = nullptr;
#endif

extern void
SK_CEGUI_RelocateLog (void);

extern void
SK_CEGUI_InitBase (void);

static volatile LONG __cegui_frames_drawn = 0L;

void ResetCEGUI_GL (void)
{
#ifndef SK_BUILD__INSTALLER
  if (! config.cegui.enable)
    return;

  // TOOD: Eliminate the stupid magic number, make this an option if it's something
  //         that is really going to stay here long-term.
  if (static_cast <ULONG> (ReadAcquire (&__cegui_frames_drawn)) < 5)
    return;

  if (cegGL == nullptr)
  {
    try {
      cegGL = reinterpret_cast <CEGUI::OpenGL3Renderer *> (
        &CEGUI::OpenGL3Renderer::bootstrapSystem ()
      );

      cegGL->enableExtraStateSettings (true);

      SK_CEGUI_RelocateLog ();
    } catch (...) {
      cegGL = nullptr;
      return;
    }

    SK_CEGUI_InitBase ();

    SK_PopupManager::getInstance ()->destroyAllPopups       ();
    SK_TextOverlayManager::getInstance ()->resetAllOverlays (cegGL);

    extern void SK_Steam_ClearPopups (void);
    SK_Steam_ClearPopups ();
  }
#endif
}

extern "C"
{

typedef void (WINAPI *finish_pfn)(void);

/* Pixel format descriptor */
typedef struct tagPIXELFORMATDESCRIPTOR
{
  WORD  nSize;
  WORD  nVersion;
  DWORD dwFlags;
  BYTE  iPixelType;
  BYTE  cColorBits;
  BYTE  cRedBits;
  BYTE  cRedShift;
  BYTE  cGreenBits;
  BYTE  cGreenShift;
  BYTE  cBlueBits;
  BYTE  cBlueShift;
  BYTE  cAlphaBits;
  BYTE  cAlphaShift;
  BYTE  cAccumBits;
  BYTE  cAccumRedBits;
  BYTE  cAccumGreenBits;
  BYTE  cAccumBlueBits;
  BYTE  cAccumAlphaBits;
  BYTE  cDepthBits;
  BYTE  cStencilBits;
  BYTE  cAuxBuffers;
  BYTE  iLayerType;
  BYTE  bReserved;
  DWORD dwLayerMask;
  DWORD dwVisibleMask;
  DWORD dwDamageMask;
} PIXELFORMATDESCRIPTOR, *PPIXELFORMATDESCRIPTOR, FAR *LPPIXELFORMATDESCRIPTOR;

__declspec (noinline)
BOOL
WINAPI
SwapBuffers (HDC hDC);

__declspec (noinline)
BOOL
WINAPI
wglSwapBuffers (HDC hDC);

extern "C++" void SK_BootOpenGL (void);

static std::queue <DWORD> old_threads;

void
WINAPI
opengl_init_callback (finish_pfn finish)
{
  SK_BootOpenGL ();

  finish ();
}

}


HMODULE local_gl = 0;

HMODULE
SK_LoadRealGL (void)
{
  wchar_t wszBackendDLL [MAX_PATH + 2] = { };

  if (! SK_IsInjected ())
  {
    wcsncpy  (wszBackendDLL, SK_GetSystemDirectory (), MAX_PATH);
    lstrcatW (wszBackendDLL, L"\\");
  }

  lstrcatW (wszBackendDLL, L"OpenGL32.dll");

  if (local_gl == 0)
    local_gl = LoadLibraryW_Original (wszBackendDLL);
  else {
    HMODULE hMod;
    GetModuleHandleEx (0x00, wszBackendDLL, &hMod);
  }

  return local_gl;
}

void
SK_FreeRealGL (void)
{
  FreeLibrary_Original (local_gl);
}


bool
SK::OpenGL::Startup (void)
{
  return SK_StartupCore (L"OpenGL32", opengl_init_callback);
}

bool
SK::OpenGL::Shutdown (void)
{
  return SK_ShutdownCore (L"OpenGL32");
}

//#include <GL/gl.h>

extern "C"
{

#define OPENGL_STUB(_Return, _Name, _Proto, _Args)                          \
    typedef _Return (WINAPI *imp_##_Name##_pfn) _Proto;                     \
    imp_##_Name##_pfn imp_##_Name = nullptr;                                \
                                                                            \
  _Return WINAPI                                                            \
  _Name _Proto {                                                            \
    if (imp_##_Name == nullptr) {                                           \
                                                                            \
      static const char* szName = #_Name;                                   \
      imp_##_Name = (imp_##_Name##_pfn)GetProcAddress (backend_dll, szName);\
                                                                            \
      if (imp_##_Name == nullptr) {                                         \
        dll_log.Log (                                                       \
          L"[ OpenGL32 ] Unable to locate symbol  %s in OpenGL32.dll",      \
          L#_Name);                                                         \
        return 0;                                                           \
      }                                                                     \
    }                                                                       \
                                                                            \
    return imp_##_Name _Args;                                               \
}

#define OPENGL_STUB_(_Name, _Proto, _Args)                                  \
    typedef void (WINAPI *imp_##_Name##_pfn) _Proto;                        \
    imp_##_Name##_pfn imp_##_Name = nullptr;                                \
                                                                            \
  void WINAPI                                                               \
  _Name _Proto {                                                            \
    if (imp_##_Name == nullptr) {                                           \
                                                                            \
      static const char* szName = #_Name;                                   \
      imp_##_Name = (imp_##_Name##_pfn)GetProcAddress (backend_dll, szName);\
                                                                            \
      if (imp_##_Name == nullptr) {                                         \
        dll_log.Log (                                                       \
          L"[ OpenGL32 ] Unable to locate symbol  %s in OpenGL32.dll",      \
          L#_Name);                                                         \
        return;                                                             \
      }                                                                     \
    }                                                                       \
                                                                            \
    imp_##_Name _Args;                                                      \
}

#include <cstdint>

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
OPENGL_STUB(HGLRC,wglCreateContext, (HDC hDC),
                                    (    hDC));


OPENGL_STUB(HGLRC,wglCreateLayerContext, (HDC hDC, int idx),
                                         (    hDC,     idx));
OPENGL_STUB(BOOL, wglDeleteContext,      (HGLRC hglrc),
                                         (      hglrc));
OPENGL_STUB(HGLRC,wglGetCurrentContext,  (VOID), ());
OPENGL_STUB(HDC,  wglGetCurrentDC,       (VOID), ());
OPENGL_STUB(PROC, wglGetProcAddress,     (LPCSTR str),
                                         (       str));
OPENGL_STUB(BOOL, wglMakeCurrent,        (HDC hDC, HGLRC hglrc),
                                         (    hDC,       hglrc));
OPENGL_STUB(BOOL, wglShareLists,         (HGLRC hglrc1, HGLRC hglrc2),
                                         (      hglrc1,       hglrc2));
OPENGL_STUB(BOOL, wglUseFontBitmapsA, (HDC hDC, DWORD dw0, DWORD dw1, DWORD dw2),
                                      (    hDC,       dw0,       dw1,       dw2));
OPENGL_STUB(BOOL, wglUseFontBitmapsW, (HDC hDC, DWORD dw0, DWORD dw1, DWORD dw2),
                                      (    hDC,       dw0,       dw1,       dw2));

OPENGL_STUB(INT, wglChoosePixelFormat, (HDC hDC, CONST PIXELFORMATDESCRIPTOR *pfd),
                                       (    hDC,                              pfd));

/* Layer plane descriptor */
typedef struct tagLAYERPLANEDESCRIPTOR { // lpd
  WORD  nSize;
  WORD  nVersion;
  DWORD dwFlags;
  BYTE  iPixelType;
  BYTE  cColorBits;
  BYTE  cRedBits;
  BYTE  cRedShift;
  BYTE  cGreenBits;
  BYTE  cGreenShift;
  BYTE  cBlueBits;
  BYTE  cBlueShift;
  BYTE  cAlphaBits;
  BYTE  cAlphaShift;
  BYTE  cAccumBits;
  BYTE  cAccumRedBits;
  BYTE  cAccumGreenBits;
  BYTE  cAccumBlueBits;
  BYTE  cAccumAlphaBits;
  BYTE  cDepthBits;
  BYTE  cStencilBits;
  BYTE  cAuxBuffers;
  BYTE  iLayerPlane;
  BYTE  bReserved;
  COLORREF crTransparent;
} LAYERPLANEDESCRIPTOR, *PLAYERPLANEDESCRIPTOR, FAR *LPLAYERPLANEDESCRIPTOR;

OPENGL_STUB(BOOL, wglDescribeLayerPlane, (HDC hDC, DWORD PixelFormat, DWORD LayerPlane, UINT nBytes, LPLAYERPLANEDESCRIPTOR lpd),
                                         (    hDC,       PixelFormat,       LayerPlane,      nBytes,                        lpd));

OPENGL_STUB(DWORD, wglDescribePixelFormat, (HDC hDC, DWORD PixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR pfd),
                                           (    hDC,       PixelFormat,      nBytes,                         pfd));

OPENGL_STUB(DWORD, wglGetLayerPaletteEntries, (HDC hDC, DWORD LayerPlane, DWORD Start, DWORD Entries, COLORREF *cr),
                                              (    hDC,       LayerPlane,       Start,       Entries,           cr));

OPENGL_STUB(BOOL, wglGetPixelFormat, (HDC hDC, DWORD iPixelFormat, DWORD iLayerPlane, UINT nAttributes, DWORD *piAttributes, DWORD *pValues),
                                     (    hDC,       iPixelFormat,       iLayerPlane,      nAttributes,        piAttributes,        pValues));

OPENGL_STUB(BOOL, wglRealizeLayerPalette, (HDC hDC, DWORD LayerPlane, BOOL Realize),
                                          (    hDC,       LayerPlane,      Realize));

OPENGL_STUB(DWORD, wglSetLayerPaletteEntries, (HDC hDC, DWORD LayerPlane, DWORD Start, DWORD Entries, CONST COLORREF *cr),
                                              (    hDC,       LayerPlane,       Start,       Entries,                 cr));

OPENGL_STUB(BOOL, wglSetPixelFormat, (HDC hDC, DWORD PixelFormat, CONST PIXELFORMATDESCRIPTOR *pdf),
                                     (    hDC,       PixelFormat,                              pdf));


void
SK_CEGUI_DrawGL (void)
{
  if (! config.cegui.enable)
    return;

  InterlockedIncrement (&__cegui_frames_drawn);

  static HWND last_hwnd = game_window.hWnd;

  if (last_hwnd != game_window.hWnd)
  {
    if (cegGL != nullptr)
    {
      CEGUI::WindowManager::getDllSingleton ().cleanDeadPool ();
      cegGL->destroySystem ();
      cegGL = nullptr;
    }

    last_hwnd = game_window.hWnd;
  }

  // TODO: Create a secondary context that shares "display lists" so that
  //         we have a pure state machine all to ourselves.
  if (cegGL == nullptr)
  {
    extern void
    SK_InstallWindowHook (HWND hWnd);
    SK_InstallWindowHook (WindowFromDC (wglGetCurrentDC ()));

    ResetCEGUI_GL ();
  }

  else
  {
    static RECT rect     = { -1, -1, -1, -1 };
           RECT rect_now = {  0,  0,  0,  0 };

    GetClientRect (WindowFromDC (wglGetCurrentDC ()), &rect_now);

    if (memcmp (&rect, &rect_now, sizeof RECT))
    {
      CEGUI::System::getDllSingleton ().getRenderer ()->setDisplaySize (
          CEGUI::Sizef (
            static_cast <float> (rect_now.right - rect_now.left),
              static_cast <float> (rect_now.bottom - rect_now.top)
          )
      );

      SK_TextOverlayManager::getInstance ()->resetAllOverlays (cegGL);

      rect = rect_now;
    }

    // Backup GL state
    GLint     last_program;              glGetIntegerv (GL_CURRENT_PROGRAM,              &last_program);
    GLint     last_texture;              glGetIntegerv (GL_TEXTURE_BINDING_2D,           &last_texture);
    GLint     last_active_texture;       glGetIntegerv (GL_ACTIVE_TEXTURE,               &last_active_texture);
    GLint     last_drawbuffer;           glGetIntegerv (GL_DRAW_FRAMEBUFFER_BINDING,     &last_drawbuffer);
    GLint     last_readbuffer;           glGetIntegerv (GL_DRAW_FRAMEBUFFER_BINDING,     &last_readbuffer);
    GLint     last_framebuffer;          glGetIntegerv (GL_FRAMEBUFFER_BINDING,          &last_framebuffer);
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

    // Do not touch the default VAO state (assuming the context even has one)
    static GLuint ceGL_VAO = 0;
              if (ceGL_VAO == 0 || (! glIsVertexArray (ceGL_VAO))) glGenVertexArrays (0, &ceGL_VAO);

    glBindVertexArray (ceGL_VAO);
    glBindFramebuffer (GL_FRAMEBUFFER, 0);

    cegGL->beginRendering   ();
    {
      SK_TextOverlayManager::getInstance ()->drawAllOverlays (0.0f, 0.0f);
      CEGUI::System::getDllSingleton ().renderAllGUIContexts ();

      SK_ImGui_DrawFrame (0x00, nullptr);

      if (SK_Steam_DrawOSD ())
      {
        CEGUI::System::getDllSingleton ().renderAllGUIContexts ();
      }
    }
    cegGL->endRendering     ();

    glActiveTexture         (last_active_texture);
    glBindTexture           (GL_TEXTURE_2D,           last_texture);

    // Bind stat encapsulation objects
    glUseProgram            (                         last_program);
    glBindVertexArray       (                         last_vertex_array);
    glBindFramebuffer       (GL_FRAMEBUFFER,          last_framebuffer);

    if (last_readbuffer != last_framebuffer)
      glBindFramebuffer     (GL_READ_BUFFER,          last_readbuffer);
    if (last_drawbuffer != last_framebuffer)
      glBindFramebuffer     (GL_DRAW_BUFFER,          last_drawbuffer);

    // TODO: Shader Pipeline Objects (the above objects fully capture all cocos2d state encapsulation)

    glBlendEquationSeparate (last_blend_equation_rgb, last_blend_equation_alpha);
    glBlendFunc             (last_blend_src,          last_blend_dst);

    if (last_enable_blend)        glEnable (GL_BLEND);        else glDisable (GL_BLEND);
    if (last_enable_cull_face)    glEnable (GL_CULL_FACE);    else glDisable (GL_CULL_FACE);
    if (last_enable_depth_test)   glEnable (GL_DEPTH_TEST);   else glDisable (GL_DEPTH_TEST);
    if (last_enable_scissor_test) glEnable (GL_SCISSOR_TEST); else glDisable (GL_SCISSOR_TEST);

    glViewport ( last_viewport    [0], last_viewport    [1], (GLsizei)last_viewport    [2], (GLsizei)last_viewport    [3]);
    glScissor  ( last_scissor_box [0], last_scissor_box [1], (GLsizei)last_scissor_box [2], (GLsizei)last_scissor_box [3]);
  }
}

#if 1
// THIS IS THE ONLY THING WE CARE ABOUT, GOOD GRIEF!!!
__declspec (noinline)
BOOL
WINAPI
SwapBuffers (HDC hDC)
{
  WaitForInit_GL ();


  // Setup our window message hook for the command console
  if (hWndRender == 0 || (! IsWindow (hWndRender)))
    hWndRender = WindowFromDC (hDC);


  SK_GetCurrentRenderBackend ().api = SK_RenderAPI::OpenGL;
  SK_BeginBufferSwap ();

  if (gdi_swap_buffers == nullptr)
  {
    HMODULE hModGDI32 = LoadLibraryW_Original (L"gdi32.dll");

    gdi_swap_buffers =
      reinterpret_cast <SwapBuffers_pfn> (
        GetProcAddress (hModGDI32, "SwapBuffers")
      );
  }


  if (SK_GetCurrentGLContext ())
  {
    if (SK_GetFramesDrawn () < 1)
    {
      glewExperimental = GL_TRUE;
      glewInit ();
    }

    SK_GL_UpdateRenderStats ();
    SK_CEGUI_DrawGL         ();

    static bool first = true;
    if (first)
    {
      ImGui_ImplGL3_Init ();
      first = false;
    }
  }


  BOOL status = FALSE;

  if (gdi_swap_buffers != nullptr)
    status = gdi_swap_buffers (hDC);

  SK_EndBufferSwap (S_OK);


  return status;
}

__declspec (noinline)
BOOL
WINAPI
wglSwapBuffers (HDC hDC)
{
  WaitForInit_GL ();


  // Setup our window message hook for the command console
  if (hWndRender == 0 || (! IsWindow (hWndRender)))
    hWndRender = WindowFromDC (hDC);

  SK_GetCurrentRenderBackend ().api = SK_RenderAPI::OpenGL;
  SK_BeginBufferSwap ();

  if (wgl_swap_buffers == nullptr)
  {
    wgl_swap_buffers =
      (wglSwapBuffers_pfn)GetProcAddress (backend_dll, "wglSwapBuffers");
  }


  if (SK_GetCurrentGLContext ())
  {
    if (SK_GetFramesDrawn () < 1)
    {
      glewExperimental = GL_TRUE;
      glewInit ();
    }

    SK_GL_UpdateRenderStats ();
    SK_CEGUI_DrawGL         ();

    static bool first = true;
    if (first)
    {
      ImGui_ImplGL3_Init ();
      first = false;
    }
  }


  BOOL status = FALSE;

  if (wgl_swap_buffers != nullptr)
    status = wgl_swap_buffers (hDC);

  SK_EndBufferSwap (S_OK);

  return status;
}

typedef struct _WGLSWAP
{
  HDC  hDC;
  UINT uiFlags;
} WGLSWAP;

typedef DWORD (WINAPI *wglSwapMultipleBuffers_pfn)(UINT n, CONST WGLSWAP *ps);

wglSwapMultipleBuffers_pfn imp_wglSwapMultipleBuffers = nullptr;

__declspec (noinline)
DWORD
WINAPI
wglSwapMultipleBuffers (UINT n, const WGLSWAP* ps)
{
  WaitForInit_GL ();

  for (UINT i = 0; i < n; i++)
    SwapBuffers (ps->hDC);

  return 0;
}

#else
OPENGL_STUB(BOOL,    SwapBuffers, (HDC hDC), (hDC));
OPENGL_STUB(BOOL, wglSwapBuffers, (HDC hDC), (hDC));
#endif

OPENGL_STUB(BOOL, wglSwapLayerBuffers, ( HDC hDC, UINT nPlanes ),
                                       (     hDC,      nPlanes ));


typedef struct _POINTFLOAT {
  FLOAT   x;
  FLOAT   y;
} POINTFLOAT, *PPOINTFLOAT;

typedef struct _GLYPHMETRICSFLOAT {
  FLOAT       gmfBlackBoxX;
  FLOAT       gmfBlackBoxY;
  POINTFLOAT  gmfptGlyphOrigin;
  FLOAT       gmfCellIncX;
  FLOAT       gmfCellIncY;
} GLYPHMETRICSFLOAT, *PGLYPHMETRICSFLOAT, FAR *LPGLYPHMETRICSFLOAT;

OPENGL_STUB(BOOL,wglUseFontOutlinesA,(HDC hDC, DWORD dw0, DWORD dw1, DWORD dw2, FLOAT f0, FLOAT f1, int i0, LPGLYPHMETRICSFLOAT pgmf),
                                     (    hDC,       dw0,       dw1,       dw2,       f0,       f1,     i0,                     pgmf));
OPENGL_STUB(BOOL,wglUseFontOutlinesW,(HDC hDC, DWORD dw0, DWORD dw1, DWORD dw2, FLOAT f0, FLOAT f1, int i0, LPGLYPHMETRICSFLOAT pgmf),
                                     (    hDC,       dw0,       dw1,       dw2,       f0,       f1,     i0,                     pgmf));

}



//#define GL_TRUE  TRUE
//#define GL_FALSE FALSE

#define GL_QUERY_RESULT           0x8866
#define GL_QUERY_RESULT_AVAILABLE 0x8867
#define GL_QUERY_RESULT_NO_WAIT   0x9194

#define GL_TIMESTAMP              0x8E28
#define GL_TIME_ELAPSED           0x88BF

typedef int64_t  GLint64;
typedef uint64_t GLuint64;

typedef GLvoid    (WINAPI *glGenQueries_pfn)   (GLsizei n​,       GLuint *ids​);
typedef GLvoid    (WINAPI *glDeleteQueries_pfn)(GLsizei n​, const GLuint *ids​);

typedef GLvoid    (WINAPI *glBeginQuery_pfn)       (GLenum target​, GLuint id​);
typedef GLvoid    (WINAPI *glBeginQueryIndexed_pfn)(GLenum target​, GLuint index​, GLuint id​);
typedef GLvoid    (WINAPI *glEndQuery_pfn)         (GLenum target​);

typedef GLboolean (WINAPI *glIsQuery_pfn)            (GLuint id​);
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
    PipelineQuery (const wchar_t* wszName, GLenum target)
    {
      name_ = wszName;
      glGenQueries (1, &query_);

      finished_  = GL_FALSE;
      ready_     = GL_TRUE;

      target_    = target;
      active_    = GL_FALSE;
    }

   ~PipelineQuery (GLvoid)
   {
      if (glIsQuery (query_))
      {
        glDeleteQueries (1, &query_);
        query_ = 0;
      }
    }

    std::wstring getName         (GLvoid)
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
    GLboolean isReady            (GLvoid)
    {
      return ready_;
    }
    GLboolean isActive           (GLvoid)
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

    GLuint64 getLastResult (void)
    {
      return result_;
    }

  protected:
    GLboolean    active_;
    GLboolean    finished_; // Has GL given us the data yet?
    GLboolean    ready_;    // Has the old value has been retrieved yet?

    GLuint64     result_;   // Cached result

  private:
    GLuint       query_;    // GL Name of Query Object
    GLenum       target_;   // Target;
    std::wstring name_;     // Human-readable name
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

  HAS_pipeline_query = true;

#if 0
    glGenQueries && glDeleteQueries && glBeginQuery       && glBeginQueryIndexed  && glEndQuery &&
    glIsQuery    && glQueryCounter  && glGetQueryObjectiv && glGetQueryObjecti64v && glGetQueryObjectui64v;
#endif

  if (HAS_pipeline_query) {
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

#include <SpecialK/config.h>

#include <d3d11.h>

void
__stdcall
SK_GL_UpdateRenderStats (void)
{
  if (! (config.render.show))
    return;

  static bool init = false;

  if (! init)
  {
    GLPerf::Init ();
    init = true;
  }

  //SK::DXGI::PipelineStatsD3D11& pipeline_stats =
    //SK::DXGI::pipeline_stats_d3d11;

  for (int i = 0; i < 11; i++)
  {
    if (GLPerf::pipeline_states [i] != nullptr)
    {
      if (GLPerf::pipeline_states [i]->isReady ())
      {
        GLuint64 result;
        if (GLPerf::pipeline_states [i]->getResulIfFinished (&result))
        {
          GLPerf::pipeline_states [i]->beginQuery ();
        }
      }

      else if (GLPerf::pipeline_states [i]->isActive ())
      {
        GLPerf::pipeline_states [i]->endQuery ();
      }

      //if (! GLPerf::pipeline_states [i]->isActive ())
      //{
      //
      //}
    }
  }
}

extern std::wstring
SK_CountToString (uint64_t count);

std::wstring
SK::OpenGL::getPipelineStatsDesc (void)
{
  wchar_t wszDesc [1024];

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
    _swprintf ( wszDesc,
                  L"  VERTEX : %s   (%s Verts ==> %s Triangles)\n",
                    SK_CountToString (stats.VSInvocations).c_str (),
                      SK_CountToString (stats.IAVertices).c_str (),
                        SK_CountToString (stats.IAPrimitives).c_str () );
  }

  else
  {
    _swprintf ( wszDesc,
                  L"  VERTEX : <Unused>\n" );
  }

  //
  // GEOMETRY SHADING
  //
  if (stats.GSInvocations > 0)
  {
    _swprintf ( wszDesc,
                  L"%s  GEOM   : %s   (%s Prims)\n",
                    wszDesc,
                      SK_CountToString (stats.GSInvocations).c_str (),
                        SK_CountToString (stats.GSPrimitives).c_str () );
  }

  else
  {
    _swprintf ( wszDesc,
                  L"%s  GEOM   : <Unused>\n",
                    wszDesc );
  }

  //
  // TESSELLATION
  //
  if (stats.HSInvocations > 0 || stats.DSInvocations > 0)
  {
    _swprintf ( wszDesc,
                  L"%s  TESS   : %s Hull ==> %s Domain\n",
                    wszDesc,
                      SK_CountToString (stats.HSInvocations).c_str (),
                        SK_CountToString (stats.DSInvocations).c_str () ) ;
  }

  else
  {
    _swprintf ( wszDesc,
                  L"%s  TESS   : <Unused>\n",
                    wszDesc );
  }

  //
  // RASTERIZATION
  //
  if (stats.CInvocations > 0)
  {
    _swprintf ( wszDesc,
                  L"%s  RASTER : %5.1f%% Filled     (%s Triangles IN )\n",
                    wszDesc, 100.0f *
                        ( static_cast <float> (stats.CPrimitives) /
                          static_cast <float> (stats.CInvocations) ),
                      SK_CountToString (stats.CInvocations).c_str () );
  }

  else
  {
    _swprintf ( wszDesc,
                  L"%s  RASTER : <Unused>\n",
                    wszDesc );
  }

  //
  // PIXEL SHADING
  //
  if (stats.PSInvocations > 0)
  {
    _swprintf ( wszDesc,
                  L"%s  PIXEL  : %s   (%s Triangles OUT)\n",
                    wszDesc,
                      SK_CountToString (stats.PSInvocations).c_str (),
                        SK_CountToString (stats.CPrimitives).c_str () );
  }

  else
  {
    _swprintf ( wszDesc,
                  L"%s  PIXEL  : <Unused>\n",
                    wszDesc );
  }

  //
  // COMPUTE
  //
  if (stats.CSInvocations > 0)
  {
    _swprintf ( wszDesc,
                  L"%s  COMPUTE: %s\n",
                    wszDesc, SK_CountToString (stats.CSInvocations).c_str () );
  }

  else
  {
    _swprintf ( wszDesc,
                  L"%s  COMPUTE: <Unused>\n",
                    wszDesc );
  }

  return wszDesc;
}

#define SK_DLL_HOOK(Backend,Func)                          \
  SK_CreateDLLHook2 ( Backend,                             \
                     #Func,                                \
                      Func,                                \
    static_cast_p2p <void> (&imp_ ## Func) ); ++GL_HOOKS;

#define SK_GL_HOOK(Func) SK_DLL_HOOK(wszBackendDLL,Func)



void
WINAPI
SK_HookGL (void)
{
  static volatile ULONG hooked = FALSE;

  if (InterlockedCompareExchange (&hooked, TRUE, FALSE))
    return;

  if (! config.apis.OpenGL.hook)
    return;

  dll_log.Log (L"[ OpenGL32 ] Additional OpenGL Initialization");
  dll_log.Log (L"[ OpenGL32 ] ================================");

  if (! StrStrIW ( SK_GetModuleName (SK_GetDLL ()).c_str (), 
                   L"OPENGL32.dll" ) )
  {
    dll_log.Log (L"[ OpenGL32 ] Hooking OpenGL");

    if (true)
    {
      wchar_t* wszBackendDLL = L"OpenGL32.dll";

#if 0
      SK_CreateDLLHook (         wszBackendDLL,
                                "wglSwapBuffers",
                                 wglSwapBuffers,
        static_cast_p2p <void> (&wgl_swap_buffers) );
#else
      SK_CreateDLLHook (       L"gdi32.dll",
                                "SwapBuffers",
                                 SwapBuffers,
        static_cast_p2p <void> (&gdi_swap_buffers) );
#endif


// Load user-defined DLLs (Plug-In)
#ifdef _WIN64
    SK_LoadPlugIns64 ();
#else
    SK_LoadPlugIns32 ();
#endif

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
      SK_GL_HOOK(wglCreateLayerContext);
      SK_GL_HOOK(wglDeleteContext);
      SK_GL_HOOK(wglGetCurrentContext);
      SK_GL_HOOK(wglGetCurrentDC);

      SK_GL_HOOK(wglGetProcAddress);
      SK_GL_HOOK(wglCreateContext);

      SK_GL_HOOK(wglChoosePixelFormat);

      SK_GL_HOOK(wglMakeCurrent);
      SK_GL_HOOK(wglShareLists);
      SK_GL_HOOK(wglUseFontBitmapsA);
      SK_GL_HOOK(wglUseFontBitmapsW);
      SK_GL_HOOK(wglDescribeLayerPlane);
      SK_GL_HOOK(wglDescribePixelFormat);
      SK_GL_HOOK(wglGetLayerPaletteEntries);
      SK_GL_HOOK(wglGetPixelFormat);
      SK_GL_HOOK(wglRealizeLayerPalette);
      SK_GL_HOOK(wglSetLayerPaletteEntries);
      SK_GL_HOOK(wglSetPixelFormat);
      SK_GL_HOOK(wglSwapMultipleBuffers);

      MH_ApplyQueued ();
    }

    dll_log.Log ( L"[ OpenGL32 ]  @ %lu functions hooked",
                    GL_HOOKS );
  }

  InterlockedExchange (&__gl_ready, TRUE);

#if 0
  _beginthreadex ( nullptr,
                     0,
                       DXGI_Thread,
                         nullptr,
                           0x00,
                             nullptr );
#endif

  GL_HOOKED = TRUE;
}


HGLRC
WINAPI
SK_GetCurrentGLContext (void)
{
  if (imp_wglGetCurrentContext != nullptr)
    return imp_wglGetCurrentContext ();

  // Fallback, better hope this never happens ;)
  return wglGetCurrentContext ();
}