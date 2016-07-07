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
#include "stdafx.h"
//#include <Windows.h>

#include "opengl_backend.h"
#include "log.h"

#include "core.h"

// Trivial in OpenGL
//#define WaitForInit() 

extern void WaitForInit (void);
extern bool SK_InitCOM (void);

extern "C"
{

typedef void (WINAPI *finish_pfn)(void);

DWORD
WINAPI
opengl_init_thread (LPVOID lpUser)
{
  SK_InitCOM ();

  dll_log.Log (L"[ OpenGL32 ] Doing things that look odd in GL");
  dll_log.Log (L"[ OpenGL32 ] ================================");

  typedef HRESULT (STDMETHODCALLTYPE *CreateDXGIFactory_t)(REFIID,IDXGIFactory**);

  static HMODULE hDXGI = LoadLibrary (L"dxgi.dll");
  static CreateDXGIFactory_t CreateDXGIFactory =
    (CreateDXGIFactory_t)GetProcAddress (hDXGI, "CreateDXGIFactory");

  IDXGIFactory* factory = nullptr;

  // Only spawn the DXGI 1.4 budget thread if ... DXGI 1.4 is implemented.
  if (SUCCEEDED (CreateDXGIFactory (__uuidof (IDXGIFactory4), &factory)) && factory != nullptr) {
    IDXGIAdapter* adapter = nullptr;

    if  (SUCCEEDED (factory->EnumAdapters (0, &adapter)) && adapter != nullptr) {
      DXGI_ADAPTER_DESC desc;

      IDXGIAdapter* adapter_original = adapter;

      if (SUCCEEDED (adapter->GetDesc (&desc))) {
        if (desc.VendorId == 0x8086) {
          dll_log.LogEx (true, L"[ DXGI 1.0 ] Skipping Intel Adapter (%s)... ", desc.Description);

          if (SUCCEEDED (factory->EnumAdapters (1, &adapter)) && adapter != nullptr) {
            if (SUCCEEDED (adapter->GetDesc (&desc))) {

              // If we found a Microsoft adapter, bail-out
              if (desc.VendorId == 0x1414) {
                dll_log.LogEx (false, L"impossible, no discrete GPU found.\n");

                adapter->Release ();

                adapter = adapter_original;
              } else {
                dll_log.LogEx ( false, L"using (%s)!\n",
                                  desc.Description );
                adapter_original->Release ();
              }
            }
          }

          else {
            adapter = adapter_original;
            dll_log.LogEx (false, L"impossible, no discrete GPU found.\n");
          }
        }
      }

      if (adapter != nullptr) {
        SK_StartDXGI_1_4_BudgetThread (&adapter);
        adapter->Release ();
      }
    }

    factory->Release ();
  }

  ((finish_pfn)lpUser) ();

  return 0;
}

void
WINAPI
opengl_init_callback (finish_pfn finish)
{
  // COM probably isn't initialized in the calling thread in a GL application
  //   like it would be in D3D...

  //HANDLE hThread =
    //CreateThread (nullptr, 0, opengl_init_thread, (LPVOID)finish, 0x00, nullptr);

  opengl_init_thread ((LPVOID)finish);
}

}


HMODULE
SK_LoadRealGL (void)
{
  wchar_t wszBackendDLL [MAX_PATH] = { L'\0' };

#ifdef _WIN64
  GetSystemDirectory (wszBackendDLL, MAX_PATH);
#else
  BOOL bWOW64;
  ::IsWow64Process (GetCurrentProcess (), &bWOW64);

  if (bWOW64)
    GetSystemWow64Directory (wszBackendDLL, MAX_PATH);
  else
    GetSystemDirectory (wszBackendDLL, MAX_PATH);
#endif

  lstrcatW (wszBackendDLL, L"\\OpenGL32.dll");

  return LoadLibraryW (wszBackendDLL);
}


bool
SK::OpenGL::Startup (void)
{
  //
  // For Thread Local Storage to work correctly, this is the only option.
  //
  //  We cannot load the system-wide DLL from a temporary thread; it must
  //    be the one that handled DLL process attach.
  //
  //   >>> This was VERY hard to debug, so please pay attention! <<<
  //
  SK_LoadRealGL ();

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

#define OPENGL_STUB(_Return, _Name, _Proto, _Args)                        \
  __declspec (nothrow)                                                    \
  _Return WINAPI                                                          \
  _Name _Proto {                                                          \
    typedef _Return (WINAPI *passthrough_t) _Proto;                       \
    static passthrough_t _default_impl = nullptr;                         \
                                                                          \
    if (_default_impl == nullptr) {                                       \
      WaitForInit ();                                                     \
                                                                          \
      while (! backend_dll)                                               \
        WaitForInit ();                                                   \
                                                                          \
      static const char* szName = #_Name;                                 \
      _default_impl = (passthrough_t)GetProcAddress (backend_dll, szName);\
                                                                          \
      if (_default_impl == nullptr) {                                     \
        dll_log.Log (                                                     \
          L"[ OpenGL32 ] Unable to locate symbol  %s in OpenGL32.dll",    \
          L#_Name);                                                       \
        return 0;                                                         \
      }                                                                   \
    }                                                                     \
                                                                          \
    return _default_impl _Args;                                           \
}

#define OPENGL_STUB_(_Name, _Proto, _Args)                                \
  __declspec (nothrow)                                                    \
  void WINAPI                                                             \
  _Name _Proto {                                                          \
    typedef void (WINAPI *passthrough_t) _Proto;                          \
    static passthrough_t _default_impl = nullptr;                         \
                                                                          \
    if (_default_impl == nullptr) {                                       \
      WaitForInit ();                                                     \
                                                                          \
      while (! backend_dll)                                               \
        WaitForInit ();                                                   \
                                                                          \
      static const char* szName = #_Name;                                 \
      _default_impl = (passthrough_t)GetProcAddress (backend_dll, szName);\
                                                                          \
      if (_default_impl == nullptr) {                                     \
        dll_log.Log (                                                     \
          L"[ OpenGL32 ] Unable to locate symbol  %s in OpenGL32.dll",    \
          L#_Name);                                                       \
        return;                                                           \
      }                                                                   \
    }                                                                     \
                                                                          \
    _default_impl _Args;                                                  \
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



#if 1
// THIS IS THE ONLY THING WE CARE ABOUT, GOOD GRIEF!!!
__declspec (noinline)
BOOL
WINAPI
SwapBuffers (HDC hDC)
{
#if 0
  // Disassembling the actual GDI library suggests this is routed through the
  //   ICD differently somehow... but proxying it through SwapBuffers seems to
  //     work fine.
  return wglSwapBuffers (hDC);
#endif
  WaitForInit ();

  SK_BeginBufferSwap ();

  typedef BOOL (WINAPI *SwapBuffers_pfn)(HDC);
  static SwapBuffers_pfn gdi_swap_buffers = nullptr;

  if (gdi_swap_buffers == nullptr) {
    HMODULE hModGDI32 = LoadLibrary (L"gdi32.dll");

    gdi_swap_buffers =
      (SwapBuffers_pfn)GetProcAddress (hModGDI32, "SwapBuffers");
  }

  BOOL status = FALSE;

  if (gdi_swap_buffers != nullptr)
    status = gdi_swap_buffers (hDC);

  SK_EndBufferSwap (S_OK);

  return status;
  //WaitForInit ();
}

__declspec (noinline)
BOOL
WINAPI
wglSwapBuffers (HDC hDC)
{
 #if 0
  // Disassembling the actual GDI library suggests this is routed through the
  //   ICD differently somehow... but proxying it through SwapBuffers seems to
  //     work fine.
  return SwapBuffers (hDC);
#endif

  WaitForInit ();

  SK_BeginBufferSwap ();

  typedef BOOL (WINAPI *wglSwapBuffers_pfn)(HDC);
  static wglSwapBuffers_pfn wgl_swap_buffers = nullptr;

  if (wgl_swap_buffers == nullptr) {
    wgl_swap_buffers =
      (wglSwapBuffers_pfn)GetProcAddress (backend_dll, "wglSwapBuffers");
  }

  BOOL status = FALSE;

  if (wgl_swap_buffers != nullptr)
    status = wgl_swap_buffers (hDC);

  SK_EndBufferSwap (S_OK);

  return status;
}
#else
OPENGL_STUB(BOOL,    SwapBuffers, (HDC hDC), (hDC));
OPENGL_STUB(BOOL, wglSwapBuffers, (HDC hDC), (hDC));
#endif

OPENGL_STUB(BOOL, wglSwapLayerBuffers, ( HDC hDC, UINT nPlanes ),
                                       (     hDC,      nPlanes ));

OPENGL_STUB(DWORD, wglSwapMultipleBuffers, ( UINT x, CONST LPVOID* y ),
                                           (      x,               y ));


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