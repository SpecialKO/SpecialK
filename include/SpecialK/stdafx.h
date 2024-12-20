// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

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

#pragma once
#pragma warning ( disable : 4652 )

#ifdef _MSC_VER
#ifndef _DEBUG
#pragma runtime_checks   ("", off)
#pragma inline_depth     (    255)
#pragma inline_recursion (     on)
#pragma vtordisp         (      2)
#pragma strict_gs_check  (    off)
#pragma check_stack -
#endif
#endif

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

//#define _CRT_NON_CONFORMING_WCSTOK
//#define WIN32_LEAN_AND_MEAN

#include "targetver.h"

#define UNICODE 1

#ifndef  NT_SUCCESS
# define NT_SUCCESS(Status)           (((NTSTATUS)(Status)) >= 0)
# define STATUS_SUCCESS                ((NTSTATUS)0x00000000L)
# define STATUS_UNSUCCESSFUL           ((NTSTATUS)0xC0000001L)
# define STATUS_INFO_LENGTH_MISMATCH   ((NTSTATUS)0xC0000004L)
//       STATUS_INVALID_PARAMETER      ((NTSTATUS)0xC000000DL)
# define STATUS_NO_SUCH_FILE           ((NTSTATUS)0xC000000FL)
# define STATUS_ACCESS_DENIED          ((NTSTATUS)0xc0000022L)
# define STATUS_BUFFER_TOO_SMALL       ((NTSTATUS)0xC0000023L)
# define STATUS_ALERTED                ((NTSTATUS)0x00000101L)
# define STATUS_PROCESS_IS_TERMINATING ((NTSTATUS)0xC000010AL)
#endif

#include <Windows.h>
#include <windowsx.h>
#include <excpt.h>

#include <wingdi.h>
#include <atldef.h>
#include <atliface.h>
#include <atlbase.h>
#include <intsafe.h>
#include <strsafe.h>
#include <comdef.h>
#include <combaseapi.h>

#include <vcruntime_exception.h>

#include <thread>
#include <numeric>
#include <ratio>
#include <mutex>
#include <chrono>
#include <cassert>
#include <typeindex>
#include <forward_list>
#include <initializer_list>

#include <intrin.h>
#include <setjmp.h>
#include <intrin0.h>

#include <vcruntime_typeinfo.h>
#include <typeinfo>

#include <locale.h>
#include <ios>
#include <iosfwd>
#include <fstream>
#include <ostream>
#include <istream>
#include <iterator>
#include <xlocnum>
#include <streambuf>
#include <xiosbase>
#include <system_error>
#include <cerrno>
#include <xcall_once.h>
#include <xerrc.h>
#include <xlocale>
#include <xfacet>
#include <stdexcept>
#include <corecrt_share.h>

#include <xatomic.h>
#include <atomic>
//#include <thr/xtimec.h>

#ifndef _DEBUG
#define GSL_UNENFORCED_ON_CONTRACT_VIOLATION
#else
#define GSL_THROW_ON_CONTRACT_VIOLATION
#endif

#include <gsl/gsl>
#include <gsl/span>
#include <gsl/pointers>
#include <gsl/gsl_util>
#include <gsl/string_span>

#include <eh.h>
#include <io.h>
#include <guiddef.h>
#include <devguid.h>
#include <setupapi.h>
#include <devpropdef.h>
#include <RegStr.h>

#include <dbt.h>
#include <wincodec.h>
#include <math.h>
#include <assert.h>
#include <dcommon.h>
#include <oleauto.h>
#include <stralign.h>
#include <mcx.h>
#include <imm.h>
#include <crtdbg.h>
#include <comip.h>
#include <shtypes.h>
#include <oleacc.h>
#include <Wbemidl.h>

#include <SpecialK/hash.h>
#include <SpecialK/crc32.h>
#include <SpecialK/utility.h>

#include <ctime>
#include <cfloat>
#include <climits>
#include <limits>
#include <numeric>
#include <functional>
#include <algorithm>

#include <string>
#include <xstring>
#include <sstream>

#include <map>
#include <set>
#include <stack>
#include <queue>
#include <array>
#include <vector>
#include <bitset>
#include <xtree>
#include <unordered_map>
#include <unordered_set>

#include <memory>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <codecvt>
#include <cinttypes>
#include <cwchar>

#include <crtdefs.h>
#include <sys/stat.h>

#define _XM_AVX_INTRINSICS_

#include <../depends/include/glm/glm.hpp>
#include <d3dcompiler.h>
#include <d3d11shader.h>
#include <DirectXMath.h>
#include <DirectXMathConvert.inl>
#include <DirectXMathVector.inl>
#include <DirectXMathMatrix.inl>
#include <DirectXMathMisc.inl>
#include <DirectXPackedVector.h>

#include <LibLoaderAPI.h>

// Fix warnings in dbghelp.h
#pragma warning   (push)
#pragma warning   (disable: 4091)
#pragma warning   (disable: 4996)
#pragma warning   (disable: 4706)
#define _IMAGEHLP_SOURCE_
#  include <DbgHelp.h>
#pragma warning   (pop)

#include <concurrent_queue.h>
#include <concurrent_vector.h>
#include <concurrent_unordered_set.h>
#include <concurrent_unordered_map.h>
#include <concurrent_priority_queue.h>

#include <SpecialK/SpecialK.h>

#include <SpecialK/injection/injection.h>
#include <SpecialK/plugin/reshade.h>

//#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#include <imgui/imgui.h>

#include <mmsystem.h>
#include <Mmdeviceapi.h>
#include <audiopolicy.h>
#include <endpointvolume.h>

#include <SpecialK/utility/bidirectional_map.h>
#include <SpecialK/osd/popup.h>

#include <lzma/7z.h>
#include <lzma/7zAlloc.h>
#include <lzma/7zBuf.h>
#include <lzma/7zCrc.h>
#include <lzma/7zFile.h>
#include <lzma/7zVersion.h>

#define SECURITY_WIN32
#include <Security.h>
#include <Wininet.h>
#include <CommCtrl.h>
#include <delayimp.h>

#include <Winver.h>
#include <objbase.h>
#include <aclapi.h>
#include <userenv.h>
#include <Shlobj.h>
#include <Shlwapi.h>
#include <shellapi.h>
#include <Unknwnbase.h>

#include <avrt.h>
#include <ntverp.h>
#include <process.h>
#include <tlhelp32.h>
#include <powrprof.h>
#include <powerbase.h>
#include <powersetting.h>
#include <WinBase.h>
#include <winnt.h>
#include <processthreadsapi.h>

#pragma comment (lib,    "avrt.lib")
#pragma comment (lib,    "secur32.lib")
#pragma comment (lib,    "wininet.lib")
#pragma comment (lib,    "Shlwapi.lib")
#pragma comment (lib,    "delayimp.lib")
#pragma comment (lib,    "wbemuuid.lib")
#pragma comment (lib,    "PowrProf.lib")

#include <SpecialK/update/version.h>
#include <SpecialK/update/archive.h>
#include <SpecialK/update/network.h>

#include <SpecialK/import.h>
#include <SpecialK/ansel.h>

#include <SpecialK/com_util.h>

#include <dinput.h>
#include <SpecialK/input/input.h>
#include <SpecialK/input/steam.h>
#include <SpecialK/input/xinput.h>
#include <SpecialK/input/xinput_hotplug.h>
#include <SpecialK/input/dinput7_backend.h>
#include <SpecialK/input/dinput8_backend.h>
#include <SpecialK/input/game_input.h>

#include <SpecialK/osd/text.h>

#include <SpecialK/sound.h>
#include <SpecialK/console.h>
#include <SpecialK/command.h>
#include <SpecialK/parameter.h>

#include <SpecialK/tls.h>
#include <SpecialK/log.h>
#include <SpecialK/ini.h>
#include <SpecialK/hooks.h>
#include <SpecialK/window.h>
#include <SpecialK/thread.h>

#include <SpecialK/steam_api.h>
#include <SpecialK/framerate.h>

#include <SpecialK/plugin/plugin_mgr.h>

#include <SpecialK/widgets/widget.h>
#include <SpecialK/control_panel.h>

#include <SpecialK/diagnostics/file.h>
#include <SpecialK/diagnostics/memory.h>
#include <SpecialK/diagnostics/network.h>
#include <SpecialK/diagnostics/modules.h>
#include <SpecialK/diagnostics/debug_utils.h>
#include <SpecialK/diagnostics/load_library.h>
#include <SpecialK/diagnostics/compatibility.h>
#include <SpecialK/diagnostics/crash_handler.h>

#include <SpecialK/performance/io_monitor.h>
#include <SpecialK/performance/gpu_monitor.h>

extern bool __SK_HDR_16BitSwap;
extern bool __SK_HDR_10BitSwap;
extern bool __SK_HDR_UserForced;

#include <dxgi1_3.h>
#include <d3d11_2.h>

#pragma region D3D9X
//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) Microsoft Corporation.  All Rights Reserved.
//
//  File:       d3dx9.h
//  Content:    D3DX utility library
//
//////////////////////////////////////////////////////////////////////////////

#ifdef  __D3DX_INTERNAL__
#error Incorrect D3DX header used
#endif

#ifndef __D3DX9_H__
#define __D3DX9_H__


// Defines
#include <limits.h>

#define D3DX_DEFAULT            ((UINT) -1)
#define D3DX_DEFAULT_NONPOW2    ((UINT) -2)
#define D3DX_DEFAULT_FLOAT      FLT_MAX
#define D3DX_FROM_FILE          ((UINT) -3)
#define D3DFMT_FROM_FILE        ((D3DFORMAT) -3)

#ifndef D3DXINLINE
#ifdef _MSC_VER
  #if (_MSC_VER >= 1200)
  #define D3DXINLINE __forceinline
  #else
  #define D3DXINLINE __inline
  #endif
#else
  #ifdef __cplusplus
  #define D3DXINLINE inline
  #else
  #define D3DXINLINE
  #endif
#endif
#endif



// Includes
#include "d3d9.h"

//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) Microsoft Corporation.  All Rights Reserved.
//
//  File:       d3dx9math.h
//  Content:    D3DX math types and functions
//
//////////////////////////////////////////////////////////////////////////////

#ifndef __D3DX9MATH_H__
#define __D3DX9MATH_H__

#include <math.h>
#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4201) // anonymous unions warning



//===========================================================================
//
// General purpose utilities
//
//===========================================================================
#define D3DX_PI    ((FLOAT)  3.141592654f)
#define D3DX_1BYPI ((FLOAT)  0.318309886f)

#define D3DXToRadian( degree ) ((degree) * (D3DX_PI / 180.0f))
#define D3DXToDegree( radian ) ((radian) * (180.0f / D3DX_PI))



//===========================================================================
//
// 16 bit floating point numbers
//
//===========================================================================

#define D3DX_16F_DIG          3                // # of decimal digits of precision
#define D3DX_16F_EPSILON      4.8875809e-4f    // smallest such that 1.0 + epsilon != 1.0
#define D3DX_16F_MANT_DIG     11               // # of bits in mantissa
#define D3DX_16F_MAX          6.550400e+004    // max value
#define D3DX_16F_MAX_10_EXP   4                // max decimal exponent
#define D3DX_16F_MAX_EXP      15               // max binary exponent
#define D3DX_16F_MIN          6.1035156e-5f    // min positive value
#define D3DX_16F_MIN_10_EXP   (-4)             // min decimal exponent
#define D3DX_16F_MIN_EXP      (-14)            // min binary exponent
#define D3DX_16F_RADIX        2                // exponent radix
#define D3DX_16F_ROUNDS       1                // addition rounding: near


typedef struct D3DXFLOAT16
{
#ifdef __cplusplus
public:
    D3DXFLOAT16() {};
    D3DXFLOAT16( FLOAT );
    D3DXFLOAT16( CONST D3DXFLOAT16& );

    // casting
    operator FLOAT ();

    // binary operators
    BOOL operator == ( CONST D3DXFLOAT16& ) const;
    BOOL operator != ( CONST D3DXFLOAT16& ) const;

protected:
#endif //__cplusplus
    WORD value;
} D3DXFLOAT16, *LPD3DXFLOAT16;



//===========================================================================
//
// Vectors
//
//===========================================================================


//--------------------------
// 2D Vector
//--------------------------
typedef struct D3DXVECTOR2
{
#ifdef __cplusplus
public:
    D3DXVECTOR2() {};
    D3DXVECTOR2( CONST FLOAT * );
    D3DXVECTOR2( CONST D3DXFLOAT16 * );
    D3DXVECTOR2( FLOAT x, FLOAT y );

    // casting
    operator FLOAT* ();
    operator CONST FLOAT* () const;

    // assignment operators
    D3DXVECTOR2& operator += ( CONST D3DXVECTOR2& );
    D3DXVECTOR2& operator -= ( CONST D3DXVECTOR2& );
    D3DXVECTOR2& operator *= ( FLOAT );
    D3DXVECTOR2& operator /= ( FLOAT );

    // unary operators
    D3DXVECTOR2 operator + () const;
    D3DXVECTOR2 operator - () const;

    // binary operators
    D3DXVECTOR2 operator + ( CONST D3DXVECTOR2& ) const;
    D3DXVECTOR2 operator - ( CONST D3DXVECTOR2& ) const;
    D3DXVECTOR2 operator * ( FLOAT ) const;
    D3DXVECTOR2 operator / ( FLOAT ) const;

    friend D3DXVECTOR2 operator * ( FLOAT, CONST D3DXVECTOR2& );

    BOOL operator == ( CONST D3DXVECTOR2& ) const;
    BOOL operator != ( CONST D3DXVECTOR2& ) const;


public:
#endif //__cplusplus
    FLOAT x, y;
} D3DXVECTOR2, *LPD3DXVECTOR2;



//--------------------------
// 2D Vector (16 bit)
//--------------------------

typedef struct D3DXVECTOR2_16F
{
#ifdef __cplusplus
public:
    D3DXVECTOR2_16F() {};
    D3DXVECTOR2_16F( CONST FLOAT * );
    D3DXVECTOR2_16F( CONST D3DXFLOAT16 * );
    D3DXVECTOR2_16F( CONST D3DXFLOAT16 &x, CONST D3DXFLOAT16 &y );

    // casting
    operator D3DXFLOAT16* ();
    operator CONST D3DXFLOAT16* () const;

    // binary operators
    BOOL operator == ( CONST D3DXVECTOR2_16F& ) const;
    BOOL operator != ( CONST D3DXVECTOR2_16F& ) const;

public:
#endif //__cplusplus
    D3DXFLOAT16 x, y;

} D3DXVECTOR2_16F, *LPD3DXVECTOR2_16F;



//--------------------------
// 3D Vector
//--------------------------
#ifdef __cplusplus
typedef struct D3DXVECTOR3 : public D3DVECTOR
{
public:
    D3DXVECTOR3() {};
    D3DXVECTOR3( CONST FLOAT * );
    D3DXVECTOR3( CONST D3DVECTOR& );
    D3DXVECTOR3( CONST D3DXFLOAT16 * );
    D3DXVECTOR3( FLOAT x, FLOAT y, FLOAT z );

    // casting
    operator FLOAT* ();
    operator CONST FLOAT* () const;

    // assignment operators
    D3DXVECTOR3& operator += ( CONST D3DXVECTOR3& );
    D3DXVECTOR3& operator -= ( CONST D3DXVECTOR3& );
    D3DXVECTOR3& operator *= ( FLOAT );
    D3DXVECTOR3& operator /= ( FLOAT );

    // unary operators
    D3DXVECTOR3 operator + () const;
    D3DXVECTOR3 operator - () const;

    // binary operators
    D3DXVECTOR3 operator + ( CONST D3DXVECTOR3& ) const;
    D3DXVECTOR3 operator - ( CONST D3DXVECTOR3& ) const;
    D3DXVECTOR3 operator * ( FLOAT ) const;
    D3DXVECTOR3 operator / ( FLOAT ) const;

    friend D3DXVECTOR3 operator * ( FLOAT, CONST struct D3DXVECTOR3& );

    BOOL operator == ( CONST D3DXVECTOR3& ) const;
    BOOL operator != ( CONST D3DXVECTOR3& ) const;

} D3DXVECTOR3, *LPD3DXVECTOR3;

#else //!__cplusplus
typedef struct _D3DVECTOR D3DXVECTOR3, *LPD3DXVECTOR3;
#endif //!__cplusplus



//--------------------------
// 3D Vector (16 bit)
//--------------------------
typedef struct D3DXVECTOR3_16F
{
#ifdef __cplusplus
public:
    D3DXVECTOR3_16F() {};
    D3DXVECTOR3_16F( CONST FLOAT * );
    D3DXVECTOR3_16F( CONST D3DVECTOR& );
    D3DXVECTOR3_16F( CONST D3DXFLOAT16 * );
    D3DXVECTOR3_16F( CONST D3DXFLOAT16 &x, CONST D3DXFLOAT16 &y, CONST D3DXFLOAT16 &z );

    // casting
    operator D3DXFLOAT16* ();
    operator CONST D3DXFLOAT16* () const;

    // binary operators
    BOOL operator == ( CONST D3DXVECTOR3_16F& ) const;
    BOOL operator != ( CONST D3DXVECTOR3_16F& ) const;

public:
#endif //__cplusplus
    D3DXFLOAT16 x, y, z;

} D3DXVECTOR3_16F, *LPD3DXVECTOR3_16F;



//--------------------------
// 4D Vector
//--------------------------
typedef struct D3DXVECTOR4
{
#ifdef __cplusplus
public:
    D3DXVECTOR4() {};
    D3DXVECTOR4( CONST FLOAT* );
    D3DXVECTOR4( CONST D3DXFLOAT16* );
    D3DXVECTOR4( CONST D3DVECTOR& xyz, FLOAT w );
    D3DXVECTOR4( FLOAT x, FLOAT y, FLOAT z, FLOAT w );

    // casting
    operator FLOAT* ();
    operator CONST FLOAT* () const;

    // assignment operators
    D3DXVECTOR4& operator += ( CONST D3DXVECTOR4& );
    D3DXVECTOR4& operator -= ( CONST D3DXVECTOR4& );
    D3DXVECTOR4& operator *= ( FLOAT );
    D3DXVECTOR4& operator /= ( FLOAT );

    // unary operators
    D3DXVECTOR4 operator + () const;
    D3DXVECTOR4 operator - () const;

    // binary operators
    D3DXVECTOR4 operator + ( CONST D3DXVECTOR4& ) const;
    D3DXVECTOR4 operator - ( CONST D3DXVECTOR4& ) const;
    D3DXVECTOR4 operator * ( FLOAT ) const;
    D3DXVECTOR4 operator / ( FLOAT ) const;

    friend D3DXVECTOR4 operator * ( FLOAT, CONST D3DXVECTOR4& );

    BOOL operator == ( CONST D3DXVECTOR4& ) const;
    BOOL operator != ( CONST D3DXVECTOR4& ) const;

public:
#endif //__cplusplus
    FLOAT x, y, z, w;
} D3DXVECTOR4, *LPD3DXVECTOR4;


//--------------------------
// 4D Vector (16 bit)
//--------------------------
typedef struct D3DXVECTOR4_16F
{
#ifdef __cplusplus
public:
    D3DXVECTOR4_16F() {};
    D3DXVECTOR4_16F( CONST FLOAT * );
    D3DXVECTOR4_16F( CONST D3DXFLOAT16* );
    D3DXVECTOR4_16F( CONST D3DXVECTOR3_16F& xyz, CONST D3DXFLOAT16& w );
    D3DXVECTOR4_16F( CONST D3DXFLOAT16& x, CONST D3DXFLOAT16& y, CONST D3DXFLOAT16& z, CONST D3DXFLOAT16& w );

    // casting
    operator D3DXFLOAT16* ();
    operator CONST D3DXFLOAT16* () const;

    // binary operators
    BOOL operator == ( CONST D3DXVECTOR4_16F& ) const;
    BOOL operator != ( CONST D3DXVECTOR4_16F& ) const;

public:
#endif //__cplusplus
    D3DXFLOAT16 x, y, z, w;

} D3DXVECTOR4_16F, *LPD3DXVECTOR4_16F;



//===========================================================================
//
// Matrices
//
//===========================================================================
#ifdef __cplusplus
typedef struct D3DXMATRIX : public D3DMATRIX
{
public:
    D3DXMATRIX() {};
    D3DXMATRIX( CONST FLOAT * );
    D3DXMATRIX( CONST D3DMATRIX& );
    D3DXMATRIX( CONST D3DXFLOAT16 * );
    D3DXMATRIX( FLOAT _11, FLOAT _12, FLOAT _13, FLOAT _14,
                FLOAT _21, FLOAT _22, FLOAT _23, FLOAT _24,
                FLOAT _31, FLOAT _32, FLOAT _33, FLOAT _34,
                FLOAT _41, FLOAT _42, FLOAT _43, FLOAT _44 );


    // access grants
    FLOAT& operator () ( UINT Row, UINT Col );
    FLOAT  operator () ( UINT Row, UINT Col ) const;

    // casting operators
    operator FLOAT* ();
    operator CONST FLOAT* () const;

    // assignment operators
    D3DXMATRIX& operator *= ( CONST D3DXMATRIX& );
    D3DXMATRIX& operator += ( CONST D3DXMATRIX& );
    D3DXMATRIX& operator -= ( CONST D3DXMATRIX& );
    D3DXMATRIX& operator *= ( FLOAT );
    D3DXMATRIX& operator /= ( FLOAT );

    // unary operators
    D3DXMATRIX operator + () const;
    D3DXMATRIX operator - () const;

    // binary operators
    D3DXMATRIX operator * ( CONST D3DXMATRIX& ) const;
    D3DXMATRIX operator + ( CONST D3DXMATRIX& ) const;
    D3DXMATRIX operator - ( CONST D3DXMATRIX& ) const;
    D3DXMATRIX operator * ( FLOAT ) const;
    D3DXMATRIX operator / ( FLOAT ) const;

    friend D3DXMATRIX operator * ( FLOAT, CONST D3DXMATRIX& );

    BOOL operator == ( CONST D3DXMATRIX& ) const;
    BOOL operator != ( CONST D3DXMATRIX& ) const;

} D3DXMATRIX, *LPD3DXMATRIX;

#else //!__cplusplus
typedef struct _D3DMATRIX D3DXMATRIX, *LPD3DXMATRIX;
#endif //!__cplusplus


//---------------------------------------------------------------------------
// Aligned Matrices
//
// This class helps keep matrices 16-byte aligned as preferred by P4 cpus.
// It aligns matrices on the stack and on the heap or in global scope.
// It does this using __declspec(align(16)) which works on VC7 and on VC 6
// with the processor pack. Unfortunately there is no way to detect the 
// latter so this is turned on only on VC7. On other compilers this is the
// the same as D3DXMATRIX.
//
// Using this class on a compiler that does not actually do the alignment
// can be dangerous since it will not expose bugs that ignore alignment.
// E.g if an object of this class in inside a struct or class, and some code
// memcopys data in it assuming tight packing. This could break on a compiler
// that eventually start aligning the matrix.
//---------------------------------------------------------------------------
#ifdef __cplusplus
typedef struct _D3DXMATRIXA16 : public D3DXMATRIX
{
    _D3DXMATRIXA16() {}
    _D3DXMATRIXA16( CONST FLOAT * );
    _D3DXMATRIXA16( CONST D3DMATRIX& );
    _D3DXMATRIXA16( CONST D3DXFLOAT16 * );
    _D3DXMATRIXA16( FLOAT _11, FLOAT _12, FLOAT _13, FLOAT _14,
                    FLOAT _21, FLOAT _22, FLOAT _23, FLOAT _24,
                    FLOAT _31, FLOAT _32, FLOAT _33, FLOAT _34,
                    FLOAT _41, FLOAT _42, FLOAT _43, FLOAT _44 );

    // new operators
    void* operator new   ( size_t );
    void* operator new[] ( size_t );

    // delete operators
    void operator delete   ( void* );   // These are NOT virtual; Do not 
    void operator delete[] ( void* );   // cast to D3DXMATRIX and delete.
    
    // assignment operators
    _D3DXMATRIXA16& operator = ( CONST D3DXMATRIX& );

} _D3DXMATRIXA16;

#else //!__cplusplus
typedef D3DXMATRIX  _D3DXMATRIXA16;
#endif //!__cplusplus



#if _MSC_VER >= 1300  // VC7
#define D3DX_ALIGN16 __declspec(align(16))
#else
#define D3DX_ALIGN16  // Earlier compiler may not understand this, do nothing.
#endif

typedef D3DX_ALIGN16 _D3DXMATRIXA16 D3DXMATRIXA16, *LPD3DXMATRIXA16;



//===========================================================================
//
//    Quaternions
//
//===========================================================================
typedef struct D3DXQUATERNION
{
#ifdef __cplusplus
public:
    D3DXQUATERNION() {}
    D3DXQUATERNION( CONST FLOAT * );
    D3DXQUATERNION( CONST D3DXFLOAT16 * );
    D3DXQUATERNION( FLOAT x, FLOAT y, FLOAT z, FLOAT w );

    // casting
    operator FLOAT* ();
    operator CONST FLOAT* () const;

    // assignment operators
    D3DXQUATERNION& operator += ( CONST D3DXQUATERNION& );
    D3DXQUATERNION& operator -= ( CONST D3DXQUATERNION& );
    D3DXQUATERNION& operator *= ( CONST D3DXQUATERNION& );
    D3DXQUATERNION& operator *= ( FLOAT );
    D3DXQUATERNION& operator /= ( FLOAT );

    // unary operators
    D3DXQUATERNION  operator + () const;
    D3DXQUATERNION  operator - () const;

    // binary operators
    D3DXQUATERNION operator + ( CONST D3DXQUATERNION& ) const;
    D3DXQUATERNION operator - ( CONST D3DXQUATERNION& ) const;
    D3DXQUATERNION operator * ( CONST D3DXQUATERNION& ) const;
    D3DXQUATERNION operator * ( FLOAT ) const;
    D3DXQUATERNION operator / ( FLOAT ) const;

    friend D3DXQUATERNION operator * (FLOAT, CONST D3DXQUATERNION& );

    BOOL operator == ( CONST D3DXQUATERNION& ) const;
    BOOL operator != ( CONST D3DXQUATERNION& ) const;

#endif //__cplusplus
    FLOAT x, y, z, w;
} D3DXQUATERNION, *LPD3DXQUATERNION;


//===========================================================================
//
// Planes
//
//===========================================================================
typedef struct D3DXPLANE
{
#ifdef __cplusplus
public:
    D3DXPLANE() {}
    D3DXPLANE( CONST FLOAT* );
    D3DXPLANE( CONST D3DXFLOAT16* );
    D3DXPLANE( FLOAT a, FLOAT b, FLOAT c, FLOAT d );

    // casting
    operator FLOAT* ();
    operator CONST FLOAT* () const;

    // assignment operators
    D3DXPLANE& operator *= ( FLOAT );
    D3DXPLANE& operator /= ( FLOAT );

    // unary operators
    D3DXPLANE operator + () const;
    D3DXPLANE operator - () const;

    // binary operators
    D3DXPLANE operator * ( FLOAT ) const;
    D3DXPLANE operator / ( FLOAT ) const;

    friend D3DXPLANE operator * ( FLOAT, CONST D3DXPLANE& );

    BOOL operator == ( CONST D3DXPLANE& ) const;
    BOOL operator != ( CONST D3DXPLANE& ) const;

#endif //__cplusplus
    FLOAT a, b, c, d;
} D3DXPLANE, *LPD3DXPLANE;


//===========================================================================
//
// Colors
//
//===========================================================================

typedef struct D3DXCOLOR
{
#ifdef __cplusplus
public:
    D3DXCOLOR() {}
    D3DXCOLOR( DWORD argb );
    D3DXCOLOR( CONST FLOAT * );
    D3DXCOLOR( CONST D3DXFLOAT16 * );
    D3DXCOLOR( CONST D3DCOLORVALUE& );
    D3DXCOLOR( FLOAT r, FLOAT g, FLOAT b, FLOAT a );

    // casting
    operator DWORD () const;

    operator FLOAT* ();
    operator CONST FLOAT* () const;

    operator D3DCOLORVALUE* ();
    operator CONST D3DCOLORVALUE* () const;

    operator D3DCOLORVALUE& ();
    operator CONST D3DCOLORVALUE& () const;

    // assignment operators
    D3DXCOLOR& operator += ( CONST D3DXCOLOR& );
    D3DXCOLOR& operator -= ( CONST D3DXCOLOR& );
    D3DXCOLOR& operator *= ( FLOAT );
    D3DXCOLOR& operator /= ( FLOAT );

    // unary operators
    D3DXCOLOR operator + () const;
    D3DXCOLOR operator - () const;

    // binary operators
    D3DXCOLOR operator + ( CONST D3DXCOLOR& ) const;
    D3DXCOLOR operator - ( CONST D3DXCOLOR& ) const;
    D3DXCOLOR operator * ( FLOAT ) const;
    D3DXCOLOR operator / ( FLOAT ) const;

    friend D3DXCOLOR operator * ( FLOAT, CONST D3DXCOLOR& );

    BOOL operator == ( CONST D3DXCOLOR& ) const;
    BOOL operator != ( CONST D3DXCOLOR& ) const;

#endif //__cplusplus
    FLOAT r, g, b, a;
} D3DXCOLOR, *LPD3DXCOLOR;



//===========================================================================
//
// D3DX math functions:
//
// NOTE:
//  * All these functions can take the same object as in and out parameters.
//
//  * Out parameters are typically also returned as return values, so that
//    the output of one function may be used as a parameter to another.
//
//===========================================================================

//--------------------------
// Float16
//--------------------------

// non-inline
#ifdef __cplusplus
extern "C" {
#endif

// Converts an array 32-bit floats to 16-bit floats
D3DXFLOAT16* WINAPI D3DXFloat32To16Array
    ( D3DXFLOAT16 *pOut, CONST FLOAT *pIn, UINT n );

// Converts an array 16-bit floats to 32-bit floats
FLOAT* WINAPI D3DXFloat16To32Array
    ( FLOAT *pOut, CONST D3DXFLOAT16 *pIn, UINT n );

#ifdef __cplusplus
}
#endif


//--------------------------
// 2D Vector
//--------------------------

// inline

FLOAT D3DXVec2Length
    ( CONST D3DXVECTOR2 *pV );

FLOAT D3DXVec2LengthSq
    ( CONST D3DXVECTOR2 *pV );

FLOAT D3DXVec2Dot
    ( CONST D3DXVECTOR2 *pV1, CONST D3DXVECTOR2 *pV2 );

// Z component of ((x1,y1,0) cross (x2,y2,0))
FLOAT D3DXVec2CCW
    ( CONST D3DXVECTOR2 *pV1, CONST D3DXVECTOR2 *pV2 );

D3DXVECTOR2* D3DXVec2Add
    ( D3DXVECTOR2 *pOut, CONST D3DXVECTOR2 *pV1, CONST D3DXVECTOR2 *pV2 );

D3DXVECTOR2* D3DXVec2Subtract
    ( D3DXVECTOR2 *pOut, CONST D3DXVECTOR2 *pV1, CONST D3DXVECTOR2 *pV2 );

// Minimize each component.  x = min(x1, x2), y = min(y1, y2)
D3DXVECTOR2* D3DXVec2Minimize
    ( D3DXVECTOR2 *pOut, CONST D3DXVECTOR2 *pV1, CONST D3DXVECTOR2 *pV2 );

// Maximize each component.  x = max(x1, x2), y = max(y1, y2)
D3DXVECTOR2* D3DXVec2Maximize
    ( D3DXVECTOR2 *pOut, CONST D3DXVECTOR2 *pV1, CONST D3DXVECTOR2 *pV2 );

D3DXVECTOR2* D3DXVec2Scale
    ( D3DXVECTOR2 *pOut, CONST D3DXVECTOR2 *pV, FLOAT s );

// Linear interpolation. V1 + s(V2-V1)
D3DXVECTOR2* D3DXVec2Lerp
    ( D3DXVECTOR2 *pOut, CONST D3DXVECTOR2 *pV1, CONST D3DXVECTOR2 *pV2,
      FLOAT s );

// non-inline
#ifdef __cplusplus
extern "C" {
#endif

D3DXVECTOR2* WINAPI D3DXVec2Normalize
    ( D3DXVECTOR2 *pOut, CONST D3DXVECTOR2 *pV );

// Hermite interpolation between position V1, tangent T1 (when s == 0)
// and position V2, tangent T2 (when s == 1).
D3DXVECTOR2* WINAPI D3DXVec2Hermite
    ( D3DXVECTOR2 *pOut, CONST D3DXVECTOR2 *pV1, CONST D3DXVECTOR2 *pT1,
      CONST D3DXVECTOR2 *pV2, CONST D3DXVECTOR2 *pT2, FLOAT s );

// CatmullRom interpolation between V1 (when s == 0) and V2 (when s == 1)
D3DXVECTOR2* WINAPI D3DXVec2CatmullRom
    ( D3DXVECTOR2 *pOut, CONST D3DXVECTOR2 *pV0, CONST D3DXVECTOR2 *pV1,
      CONST D3DXVECTOR2 *pV2, CONST D3DXVECTOR2 *pV3, FLOAT s );

// Barycentric coordinates.  V1 + f(V2-V1) + g(V3-V1)
D3DXVECTOR2* WINAPI D3DXVec2BaryCentric
    ( D3DXVECTOR2 *pOut, CONST D3DXVECTOR2 *pV1, CONST D3DXVECTOR2 *pV2,
      CONST D3DXVECTOR2 *pV3, FLOAT f, FLOAT g);

// Transform (x, y, 0, 1) by matrix.
D3DXVECTOR4* WINAPI D3DXVec2Transform
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR2 *pV, CONST D3DXMATRIX *pM );

// Transform (x, y, 0, 1) by matrix, project result back into w=1.
D3DXVECTOR2* WINAPI D3DXVec2TransformCoord
    ( D3DXVECTOR2 *pOut, CONST D3DXVECTOR2 *pV, CONST D3DXMATRIX *pM );

// Transform (x, y, 0, 0) by matrix.
D3DXVECTOR2* WINAPI D3DXVec2TransformNormal
    ( D3DXVECTOR2 *pOut, CONST D3DXVECTOR2 *pV, CONST D3DXMATRIX *pM );
     
// Transform Array (x, y, 0, 1) by matrix.
D3DXVECTOR4* WINAPI D3DXVec2TransformArray
    ( D3DXVECTOR4 *pOut, UINT OutStride, CONST D3DXVECTOR2 *pV, UINT VStride, CONST D3DXMATRIX *pM, UINT n);

// Transform Array (x, y, 0, 1) by matrix, project result back into w=1.
D3DXVECTOR2* WINAPI D3DXVec2TransformCoordArray
    ( D3DXVECTOR2 *pOut, UINT OutStride, CONST D3DXVECTOR2 *pV, UINT VStride, CONST D3DXMATRIX *pM, UINT n );

// Transform Array (x, y, 0, 0) by matrix.
D3DXVECTOR2* WINAPI D3DXVec2TransformNormalArray
    ( D3DXVECTOR2 *pOut, UINT OutStride, CONST D3DXVECTOR2 *pV, UINT VStride, CONST D3DXMATRIX *pM, UINT n );
    
    

#ifdef __cplusplus
}
#endif


//--------------------------
// 3D Vector
//--------------------------

// inline

FLOAT D3DXVec3Length
    ( CONST D3DXVECTOR3 *pV );

FLOAT D3DXVec3LengthSq
    ( CONST D3DXVECTOR3 *pV );

FLOAT D3DXVec3Dot
    ( CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2 );

D3DXVECTOR3* D3DXVec3Cross
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2 );

D3DXVECTOR3* D3DXVec3Add
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2 );

D3DXVECTOR3* D3DXVec3Subtract
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2 );

// Minimize each component.  x = min(x1, x2), y = min(y1, y2), ...
D3DXVECTOR3* D3DXVec3Minimize
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2 );

// Maximize each component.  x = max(x1, x2), y = max(y1, y2), ...
D3DXVECTOR3* D3DXVec3Maximize
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2 );

D3DXVECTOR3* D3DXVec3Scale
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV, FLOAT s);

// Linear interpolation. V1 + s(V2-V1)
D3DXVECTOR3* D3DXVec3Lerp
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2,
      FLOAT s );

// non-inline
#ifdef __cplusplus
extern "C" {
#endif

D3DXVECTOR3* WINAPI D3DXVec3Normalize
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV );

// Hermite interpolation between position V1, tangent T1 (when s == 0)
// and position V2, tangent T2 (when s == 1).
D3DXVECTOR3* WINAPI D3DXVec3Hermite
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pT1,
      CONST D3DXVECTOR3 *pV2, CONST D3DXVECTOR3 *pT2, FLOAT s );

// CatmullRom interpolation between V1 (when s == 0) and V2 (when s == 1)
D3DXVECTOR3* WINAPI D3DXVec3CatmullRom
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV0, CONST D3DXVECTOR3 *pV1,
      CONST D3DXVECTOR3 *pV2, CONST D3DXVECTOR3 *pV3, FLOAT s );

// Barycentric coordinates.  V1 + f(V2-V1) + g(V3-V1)
D3DXVECTOR3* WINAPI D3DXVec3BaryCentric
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2,
      CONST D3DXVECTOR3 *pV3, FLOAT f, FLOAT g);

// Transform (x, y, z, 1) by matrix.
D3DXVECTOR4* WINAPI D3DXVec3Transform
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR3 *pV, CONST D3DXMATRIX *pM );

// Transform (x, y, z, 1) by matrix, project result back into w=1.
D3DXVECTOR3* WINAPI D3DXVec3TransformCoord
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV, CONST D3DXMATRIX *pM );

// Transform (x, y, z, 0) by matrix.  If you transforming a normal by a 
// non-affine matrix, the matrix you pass to this function should be the 
// transpose of the inverse of the matrix you would use to transform a coord.
D3DXVECTOR3* WINAPI D3DXVec3TransformNormal
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV, CONST D3DXMATRIX *pM );
    
    
// Transform Array (x, y, z, 1) by matrix. 
D3DXVECTOR4* WINAPI D3DXVec3TransformArray
    ( D3DXVECTOR4 *pOut, UINT OutStride, CONST D3DXVECTOR3 *pV, UINT VStride, CONST D3DXMATRIX *pM, UINT n );

// Transform Array (x, y, z, 1) by matrix, project result back into w=1.
D3DXVECTOR3* WINAPI D3DXVec3TransformCoordArray
    ( D3DXVECTOR3 *pOut, UINT OutStride, CONST D3DXVECTOR3 *pV, UINT VStride, CONST D3DXMATRIX *pM, UINT n );

// Transform (x, y, z, 0) by matrix.  If you transforming a normal by a 
// non-affine matrix, the matrix you pass to this function should be the 
// transpose of the inverse of the matrix you would use to transform a coord.
D3DXVECTOR3* WINAPI D3DXVec3TransformNormalArray
    ( D3DXVECTOR3 *pOut, UINT OutStride, CONST D3DXVECTOR3 *pV, UINT VStride, CONST D3DXMATRIX *pM, UINT n );

// Project vector from object space into screen space
D3DXVECTOR3* WINAPI D3DXVec3Project
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV, CONST D3DVIEWPORT9 *pViewport,
      CONST D3DXMATRIX *pProjection, CONST D3DXMATRIX *pView, CONST D3DXMATRIX *pWorld);

// Project vector from screen space into object space
D3DXVECTOR3* WINAPI D3DXVec3Unproject
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV, CONST D3DVIEWPORT9 *pViewport,
      CONST D3DXMATRIX *pProjection, CONST D3DXMATRIX *pView, CONST D3DXMATRIX *pWorld);
      
// Project vector Array from object space into screen space
D3DXVECTOR3* WINAPI D3DXVec3ProjectArray
    ( D3DXVECTOR3 *pOut, UINT OutStride,CONST D3DXVECTOR3 *pV, UINT VStride,CONST D3DVIEWPORT9 *pViewport,
      CONST D3DXMATRIX *pProjection, CONST D3DXMATRIX *pView, CONST D3DXMATRIX *pWorld, UINT n);

// Project vector Array from screen space into object space
D3DXVECTOR3* WINAPI D3DXVec3UnprojectArray
    ( D3DXVECTOR3 *pOut, UINT OutStride, CONST D3DXVECTOR3 *pV, UINT VStride, CONST D3DVIEWPORT9 *pViewport,
      CONST D3DXMATRIX *pProjection, CONST D3DXMATRIX *pView, CONST D3DXMATRIX *pWorld, UINT n);


#ifdef __cplusplus
}
#endif



//--------------------------
// 4D Vector
//--------------------------

// inline

FLOAT D3DXVec4Length
    ( CONST D3DXVECTOR4 *pV );

FLOAT D3DXVec4LengthSq
    ( CONST D3DXVECTOR4 *pV );

FLOAT D3DXVec4Dot
    ( CONST D3DXVECTOR4 *pV1, CONST D3DXVECTOR4 *pV2 );

D3DXVECTOR4* D3DXVec4Add
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV1, CONST D3DXVECTOR4 *pV2);

D3DXVECTOR4* D3DXVec4Subtract
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV1, CONST D3DXVECTOR4 *pV2);

// Minimize each component.  x = min(x1, x2), y = min(y1, y2), ...
D3DXVECTOR4* D3DXVec4Minimize
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV1, CONST D3DXVECTOR4 *pV2);

// Maximize each component.  x = max(x1, x2), y = max(y1, y2), ...
D3DXVECTOR4* D3DXVec4Maximize
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV1, CONST D3DXVECTOR4 *pV2);

D3DXVECTOR4* D3DXVec4Scale
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV, FLOAT s);

// Linear interpolation. V1 + s(V2-V1)
D3DXVECTOR4* D3DXVec4Lerp
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV1, CONST D3DXVECTOR4 *pV2,
      FLOAT s );

// non-inline
#ifdef __cplusplus
extern "C" {
#endif

// Cross-product in 4 dimensions.
D3DXVECTOR4* WINAPI D3DXVec4Cross
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV1, CONST D3DXVECTOR4 *pV2,
      CONST D3DXVECTOR4 *pV3);

D3DXVECTOR4* WINAPI D3DXVec4Normalize
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV );

// Hermite interpolation between position V1, tangent T1 (when s == 0)
// and position V2, tangent T2 (when s == 1).
D3DXVECTOR4* WINAPI D3DXVec4Hermite
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV1, CONST D3DXVECTOR4 *pT1,
      CONST D3DXVECTOR4 *pV2, CONST D3DXVECTOR4 *pT2, FLOAT s );

// CatmullRom interpolation between V1 (when s == 0) and V2 (when s == 1)
D3DXVECTOR4* WINAPI D3DXVec4CatmullRom
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV0, CONST D3DXVECTOR4 *pV1,
      CONST D3DXVECTOR4 *pV2, CONST D3DXVECTOR4 *pV3, FLOAT s );

// Barycentric coordinates.  V1 + f(V2-V1) + g(V3-V1)
D3DXVECTOR4* WINAPI D3DXVec4BaryCentric
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV1, CONST D3DXVECTOR4 *pV2,
      CONST D3DXVECTOR4 *pV3, FLOAT f, FLOAT g);

// Transform vector by matrix.
D3DXVECTOR4* WINAPI D3DXVec4Transform
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV, CONST D3DXMATRIX *pM );
    
// Transform vector array by matrix.
D3DXVECTOR4* WINAPI D3DXVec4TransformArray
    ( D3DXVECTOR4 *pOut, UINT OutStride, CONST D3DXVECTOR4 *pV, UINT VStride, CONST D3DXMATRIX *pM, UINT n );

#ifdef __cplusplus
}
#endif


//--------------------------
// 4D Matrix
//--------------------------

// inline

D3DXMATRIX* D3DXMatrixIdentity
    ( D3DXMATRIX *pOut );

BOOL D3DXMatrixIsIdentity
    ( CONST D3DXMATRIX *pM );


// non-inline
#ifdef __cplusplus
extern "C" {
#endif

FLOAT WINAPI D3DXMatrixDeterminant
    ( CONST D3DXMATRIX *pM );

HRESULT WINAPI D3DXMatrixDecompose
    ( D3DXVECTOR3 *pOutScale, D3DXQUATERNION *pOutRotation, 
	  D3DXVECTOR3 *pOutTranslation, CONST D3DXMATRIX *pM );

D3DXMATRIX* WINAPI D3DXMatrixTranspose
    ( D3DXMATRIX *pOut, CONST D3DXMATRIX *pM );

// Matrix multiplication.  The result represents the transformation M2
// followed by the transformation M1.  (Out = M1 * M2)
D3DXMATRIX* WINAPI D3DXMatrixMultiply
    ( D3DXMATRIX *pOut, CONST D3DXMATRIX *pM1, CONST D3DXMATRIX *pM2 );

// Matrix multiplication, followed by a transpose. (Out = T(M1 * M2))
D3DXMATRIX* WINAPI D3DXMatrixMultiplyTranspose
    ( D3DXMATRIX *pOut, CONST D3DXMATRIX *pM1, CONST D3DXMATRIX *pM2 );

// Calculate inverse of matrix.  Inversion my fail, in which case NULL will
// be returned.  The determinant of pM is also returned it pfDeterminant
// is non-NULL.
D3DXMATRIX* WINAPI D3DXMatrixInverse
    ( D3DXMATRIX *pOut, FLOAT *pDeterminant, CONST D3DXMATRIX *pM );

// Build a matrix which scales by (sx, sy, sz)
D3DXMATRIX* WINAPI D3DXMatrixScaling
    ( D3DXMATRIX *pOut, FLOAT sx, FLOAT sy, FLOAT sz );

// Build a matrix which translates by (x, y, z)
D3DXMATRIX* WINAPI D3DXMatrixTranslation
    ( D3DXMATRIX *pOut, FLOAT x, FLOAT y, FLOAT z );

// Build a matrix which rotates around the X axis
D3DXMATRIX* WINAPI D3DXMatrixRotationX
    ( D3DXMATRIX *pOut, FLOAT Angle );

// Build a matrix which rotates around the Y axis
D3DXMATRIX* WINAPI D3DXMatrixRotationY
    ( D3DXMATRIX *pOut, FLOAT Angle );

// Build a matrix which rotates around the Z axis
D3DXMATRIX* WINAPI D3DXMatrixRotationZ
    ( D3DXMATRIX *pOut, FLOAT Angle );

// Build a matrix which rotates around an arbitrary axis
D3DXMATRIX* WINAPI D3DXMatrixRotationAxis
    ( D3DXMATRIX *pOut, CONST D3DXVECTOR3 *pV, FLOAT Angle );

// Build a matrix from a quaternion
D3DXMATRIX* WINAPI D3DXMatrixRotationQuaternion
    ( D3DXMATRIX *pOut, CONST D3DXQUATERNION *pQ);

// Yaw around the Y axis, a pitch around the X axis,
// and a roll around the Z axis.
D3DXMATRIX* WINAPI D3DXMatrixRotationYawPitchRoll
    ( D3DXMATRIX *pOut, FLOAT Yaw, FLOAT Pitch, FLOAT Roll );

// Build transformation matrix.  NULL arguments are treated as identity.
// Mout = Msc-1 * Msr-1 * Ms * Msr * Msc * Mrc-1 * Mr * Mrc * Mt
D3DXMATRIX* WINAPI D3DXMatrixTransformation
    ( D3DXMATRIX *pOut, CONST D3DXVECTOR3 *pScalingCenter,
      CONST D3DXQUATERNION *pScalingRotation, CONST D3DXVECTOR3 *pScaling,
      CONST D3DXVECTOR3 *pRotationCenter, CONST D3DXQUATERNION *pRotation,
      CONST D3DXVECTOR3 *pTranslation);

// Build 2D transformation matrix in XY plane.  NULL arguments are treated as identity.
// Mout = Msc-1 * Msr-1 * Ms * Msr * Msc * Mrc-1 * Mr * Mrc * Mt
D3DXMATRIX* WINAPI D3DXMatrixTransformation2D
    ( D3DXMATRIX *pOut, CONST D3DXVECTOR2* pScalingCenter, 
      FLOAT ScalingRotation, CONST D3DXVECTOR2* pScaling, 
      CONST D3DXVECTOR2* pRotationCenter, FLOAT Rotation, 
      CONST D3DXVECTOR2* pTranslation);

// Build affine transformation matrix.  NULL arguments are treated as identity.
// Mout = Ms * Mrc-1 * Mr * Mrc * Mt
D3DXMATRIX* WINAPI D3DXMatrixAffineTransformation
    ( D3DXMATRIX *pOut, FLOAT Scaling, CONST D3DXVECTOR3 *pRotationCenter,
      CONST D3DXQUATERNION *pRotation, CONST D3DXVECTOR3 *pTranslation);

// Build 2D affine transformation matrix in XY plane.  NULL arguments are treated as identity.
// Mout = Ms * Mrc-1 * Mr * Mrc * Mt
D3DXMATRIX* WINAPI D3DXMatrixAffineTransformation2D
    ( D3DXMATRIX *pOut, FLOAT Scaling, CONST D3DXVECTOR2* pRotationCenter, 
      FLOAT Rotation, CONST D3DXVECTOR2* pTranslation);

// Build a lookat matrix. (right-handed)
D3DXMATRIX* WINAPI D3DXMatrixLookAtRH
    ( D3DXMATRIX *pOut, CONST D3DXVECTOR3 *pEye, CONST D3DXVECTOR3 *pAt,
      CONST D3DXVECTOR3 *pUp );

// Build a lookat matrix. (left-handed)
D3DXMATRIX* WINAPI D3DXMatrixLookAtLH
    ( D3DXMATRIX *pOut, CONST D3DXVECTOR3 *pEye, CONST D3DXVECTOR3 *pAt,
      CONST D3DXVECTOR3 *pUp );

// Build a perspective projection matrix. (right-handed)
D3DXMATRIX* WINAPI D3DXMatrixPerspectiveRH
    ( D3DXMATRIX *pOut, FLOAT w, FLOAT h, FLOAT zn, FLOAT zf );

// Build a perspective projection matrix. (left-handed)
D3DXMATRIX* WINAPI D3DXMatrixPerspectiveLH
    ( D3DXMATRIX *pOut, FLOAT w, FLOAT h, FLOAT zn, FLOAT zf );

// Build a perspective projection matrix. (right-handed)
D3DXMATRIX* WINAPI D3DXMatrixPerspectiveFovRH
    ( D3DXMATRIX *pOut, FLOAT fovy, FLOAT Aspect, FLOAT zn, FLOAT zf );

// Build a perspective projection matrix. (left-handed)
D3DXMATRIX* WINAPI D3DXMatrixPerspectiveFovLH
    ( D3DXMATRIX *pOut, FLOAT fovy, FLOAT Aspect, FLOAT zn, FLOAT zf );

// Build a perspective projection matrix. (right-handed)
D3DXMATRIX* WINAPI D3DXMatrixPerspectiveOffCenterRH
    ( D3DXMATRIX *pOut, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn,
      FLOAT zf );

// Build a perspective projection matrix. (left-handed)
D3DXMATRIX* WINAPI D3DXMatrixPerspectiveOffCenterLH
    ( D3DXMATRIX *pOut, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn,
      FLOAT zf );

// Build an ortho projection matrix. (right-handed)
D3DXMATRIX* WINAPI D3DXMatrixOrthoRH
    ( D3DXMATRIX *pOut, FLOAT w, FLOAT h, FLOAT zn, FLOAT zf );

// Build an ortho projection matrix. (left-handed)
D3DXMATRIX* WINAPI D3DXMatrixOrthoLH
    ( D3DXMATRIX *pOut, FLOAT w, FLOAT h, FLOAT zn, FLOAT zf );

// Build an ortho projection matrix. (right-handed)
D3DXMATRIX* WINAPI D3DXMatrixOrthoOffCenterRH
    ( D3DXMATRIX *pOut, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn,
      FLOAT zf );

// Build an ortho projection matrix. (left-handed)
D3DXMATRIX* WINAPI D3DXMatrixOrthoOffCenterLH
    ( D3DXMATRIX *pOut, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn,
      FLOAT zf );

// Build a matrix which flattens geometry into a plane, as if casting
// a shadow from a light.
D3DXMATRIX* WINAPI D3DXMatrixShadow
    ( D3DXMATRIX *pOut, CONST D3DXVECTOR4 *pLight,
      CONST D3DXPLANE *pPlane );

// Build a matrix which reflects the coordinate system about a plane
D3DXMATRIX* WINAPI D3DXMatrixReflect
    ( D3DXMATRIX *pOut, CONST D3DXPLANE *pPlane );

#ifdef __cplusplus
}
#endif


//--------------------------
// Quaternion
//--------------------------

// inline

FLOAT D3DXQuaternionLength
    ( CONST D3DXQUATERNION *pQ );

// Length squared, or "norm"
FLOAT D3DXQuaternionLengthSq
    ( CONST D3DXQUATERNION *pQ );

FLOAT D3DXQuaternionDot
    ( CONST D3DXQUATERNION *pQ1, CONST D3DXQUATERNION *pQ2 );

// (0, 0, 0, 1)
D3DXQUATERNION* D3DXQuaternionIdentity
    ( D3DXQUATERNION *pOut );

BOOL D3DXQuaternionIsIdentity
    ( CONST D3DXQUATERNION *pQ );

// (-x, -y, -z, w)
D3DXQUATERNION* D3DXQuaternionConjugate
    ( D3DXQUATERNION *pOut, CONST D3DXQUATERNION *pQ );


// non-inline
#ifdef __cplusplus
extern "C" {
#endif

// Compute a quaternin's axis and angle of rotation. Expects unit quaternions.
void WINAPI D3DXQuaternionToAxisAngle
    ( CONST D3DXQUATERNION *pQ, D3DXVECTOR3 *pAxis, FLOAT *pAngle );

// Build a quaternion from a rotation matrix.
D3DXQUATERNION* WINAPI D3DXQuaternionRotationMatrix
    ( D3DXQUATERNION *pOut, CONST D3DXMATRIX *pM);

// Rotation about arbitrary axis.
D3DXQUATERNION* WINAPI D3DXQuaternionRotationAxis
    ( D3DXQUATERNION *pOut, CONST D3DXVECTOR3 *pV, FLOAT Angle );

// Yaw around the Y axis, a pitch around the X axis,
// and a roll around the Z axis.
D3DXQUATERNION* WINAPI D3DXQuaternionRotationYawPitchRoll
    ( D3DXQUATERNION *pOut, FLOAT Yaw, FLOAT Pitch, FLOAT Roll );

// Quaternion multiplication.  The result represents the rotation Q2
// followed by the rotation Q1.  (Out = Q2 * Q1)
D3DXQUATERNION* WINAPI D3DXQuaternionMultiply
    ( D3DXQUATERNION *pOut, CONST D3DXQUATERNION *pQ1,
      CONST D3DXQUATERNION *pQ2 );

D3DXQUATERNION* WINAPI D3DXQuaternionNormalize
    ( D3DXQUATERNION *pOut, CONST D3DXQUATERNION *pQ );

// Conjugate and re-norm
D3DXQUATERNION* WINAPI D3DXQuaternionInverse
    ( D3DXQUATERNION *pOut, CONST D3DXQUATERNION *pQ );

// Expects unit quaternions.
// if q = (cos(theta), sin(theta) * v); ln(q) = (0, theta * v)
D3DXQUATERNION* WINAPI D3DXQuaternionLn
    ( D3DXQUATERNION *pOut, CONST D3DXQUATERNION *pQ );

// Expects pure quaternions. (w == 0)  w is ignored in calculation.
// if q = (0, theta * v); exp(q) = (cos(theta), sin(theta) * v)
D3DXQUATERNION* WINAPI D3DXQuaternionExp
    ( D3DXQUATERNION *pOut, CONST D3DXQUATERNION *pQ );
      
// Spherical linear interpolation between Q1 (t == 0) and Q2 (t == 1).
// Expects unit quaternions.
D3DXQUATERNION* WINAPI D3DXQuaternionSlerp
    ( D3DXQUATERNION *pOut, CONST D3DXQUATERNION *pQ1,
      CONST D3DXQUATERNION *pQ2, FLOAT t );

// Spherical quadrangle interpolation.
// Slerp(Slerp(Q1, C, t), Slerp(A, B, t), 2t(1-t))
D3DXQUATERNION* WINAPI D3DXQuaternionSquad
    ( D3DXQUATERNION *pOut, CONST D3DXQUATERNION *pQ1,
      CONST D3DXQUATERNION *pA, CONST D3DXQUATERNION *pB,
      CONST D3DXQUATERNION *pC, FLOAT t );

// Setup control points for spherical quadrangle interpolation
// from Q1 to Q2.  The control points are chosen in such a way 
// to ensure the continuity of tangents with adjacent segments.
void WINAPI D3DXQuaternionSquadSetup
    ( D3DXQUATERNION *pAOut, D3DXQUATERNION *pBOut, D3DXQUATERNION *pCOut,
      CONST D3DXQUATERNION *pQ0, CONST D3DXQUATERNION *pQ1, 
      CONST D3DXQUATERNION *pQ2, CONST D3DXQUATERNION *pQ3 );

// Barycentric interpolation.
// Slerp(Slerp(Q1, Q2, f+g), Slerp(Q1, Q3, f+g), g/(f+g))
D3DXQUATERNION* WINAPI D3DXQuaternionBaryCentric
    ( D3DXQUATERNION *pOut, CONST D3DXQUATERNION *pQ1,
      CONST D3DXQUATERNION *pQ2, CONST D3DXQUATERNION *pQ3,
      FLOAT f, FLOAT g );

#ifdef __cplusplus
}
#endif


//--------------------------
// Plane
//--------------------------

// inline

// ax + by + cz + dw
FLOAT D3DXPlaneDot
    ( CONST D3DXPLANE *pP, CONST D3DXVECTOR4 *pV);

// ax + by + cz + d
FLOAT D3DXPlaneDotCoord
    ( CONST D3DXPLANE *pP, CONST D3DXVECTOR3 *pV);

// ax + by + cz
FLOAT D3DXPlaneDotNormal
    ( CONST D3DXPLANE *pP, CONST D3DXVECTOR3 *pV);

D3DXPLANE* D3DXPlaneScale
    (D3DXPLANE *pOut, CONST D3DXPLANE *pP, FLOAT s);

// non-inline
#ifdef __cplusplus
extern "C" {
#endif

// Normalize plane (so that |a,b,c| == 1)
D3DXPLANE* WINAPI D3DXPlaneNormalize
    ( D3DXPLANE *pOut, CONST D3DXPLANE *pP);

// Find the intersection between a plane and a line.  If the line is
// parallel to the plane, NULL is returned.
D3DXVECTOR3* WINAPI D3DXPlaneIntersectLine
    ( D3DXVECTOR3 *pOut, CONST D3DXPLANE *pP, CONST D3DXVECTOR3 *pV1,
      CONST D3DXVECTOR3 *pV2);

// Construct a plane from a point and a normal
D3DXPLANE* WINAPI D3DXPlaneFromPointNormal
    ( D3DXPLANE *pOut, CONST D3DXVECTOR3 *pPoint, CONST D3DXVECTOR3 *pNormal);

// Construct a plane from 3 points
D3DXPLANE* WINAPI D3DXPlaneFromPoints
    ( D3DXPLANE *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2,
      CONST D3DXVECTOR3 *pV3);

// Transform a plane by a matrix.  The vector (a,b,c) must be normal.
// M should be the inverse transpose of the transformation desired.
D3DXPLANE* WINAPI D3DXPlaneTransform
    ( D3DXPLANE *pOut, CONST D3DXPLANE *pP, CONST D3DXMATRIX *pM );
    
// Transform an array of planes by a matrix.  The vectors (a,b,c) must be normal.
// M should be the inverse transpose of the transformation desired.
D3DXPLANE* WINAPI D3DXPlaneTransformArray
    ( D3DXPLANE *pOut, UINT OutStride, CONST D3DXPLANE *pP, UINT PStride, CONST D3DXMATRIX *pM, UINT n );

#ifdef __cplusplus
}
#endif


//--------------------------
// Color
//--------------------------

// inline

// (1-r, 1-g, 1-b, a)
D3DXCOLOR* D3DXColorNegative
    (D3DXCOLOR *pOut, CONST D3DXCOLOR *pC);

D3DXCOLOR* D3DXColorAdd
    (D3DXCOLOR *pOut, CONST D3DXCOLOR *pC1, CONST D3DXCOLOR *pC2);

D3DXCOLOR* D3DXColorSubtract
    (D3DXCOLOR *pOut, CONST D3DXCOLOR *pC1, CONST D3DXCOLOR *pC2);

D3DXCOLOR* D3DXColorScale
    (D3DXCOLOR *pOut, CONST D3DXCOLOR *pC, FLOAT s);

// (r1*r2, g1*g2, b1*b2, a1*a2)
D3DXCOLOR* D3DXColorModulate
    (D3DXCOLOR *pOut, CONST D3DXCOLOR *pC1, CONST D3DXCOLOR *pC2);

// Linear interpolation of r,g,b, and a. C1 + s(C2-C1)
D3DXCOLOR* D3DXColorLerp
    (D3DXCOLOR *pOut, CONST D3DXCOLOR *pC1, CONST D3DXCOLOR *pC2, FLOAT s);

// non-inline
#ifdef __cplusplus
extern "C" {
#endif

// Interpolate r,g,b between desaturated color and color.
// DesaturatedColor + s(Color - DesaturatedColor)
D3DXCOLOR* WINAPI D3DXColorAdjustSaturation
    (D3DXCOLOR *pOut, CONST D3DXCOLOR *pC, FLOAT s);

// Interpolate r,g,b between 50% grey and color.  Grey + s(Color - Grey)
D3DXCOLOR* WINAPI D3DXColorAdjustContrast
    (D3DXCOLOR *pOut, CONST D3DXCOLOR *pC, FLOAT c);

#ifdef __cplusplus
}
#endif




//--------------------------
// Misc
//--------------------------

#ifdef __cplusplus
extern "C" {
#endif

// Calculate Fresnel term given the cosine of theta (likely obtained by
// taking the dot of two normals), and the refraction index of the material.
FLOAT WINAPI D3DXFresnelTerm
    (FLOAT CosTheta, FLOAT RefractionIndex);     

#ifdef __cplusplus
}
#endif



//===========================================================================
//
//    Matrix Stack
//
//===========================================================================

typedef interface ID3DXMatrixStack ID3DXMatrixStack;
typedef interface ID3DXMatrixStack *LPD3DXMATRIXSTACK;

// {C7885BA7-F990-4fe7-922D-8515E477DD85}
DEFINE_GUID(IID_ID3DXMatrixStack, 
0xc7885ba7, 0xf990, 0x4fe7, 0x92, 0x2d, 0x85, 0x15, 0xe4, 0x77, 0xdd, 0x85);


#undef INTERFACE
#define INTERFACE ID3DXMatrixStack

DECLARE_INTERFACE_(ID3DXMatrixStack, IUnknown)
{
    //
    // IUnknown methods
    //
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    //
    // ID3DXMatrixStack methods
    //

    // Pops the top of the stack, returns the current top
    // *after* popping the top.
    STDMETHOD(Pop)(THIS) PURE;

    // Pushes the stack by one, duplicating the current matrix.
    STDMETHOD(Push)(THIS) PURE;

    // Loads identity in the current matrix.
    STDMETHOD(LoadIdentity)(THIS) PURE;

    // Loads the given matrix into the current matrix
    STDMETHOD(LoadMatrix)(THIS_ CONST D3DXMATRIX* pM ) PURE;

    // Right-Multiplies the given matrix to the current matrix.
    // (transformation is about the current world origin)
    STDMETHOD(MultMatrix)(THIS_ CONST D3DXMATRIX* pM ) PURE;

    // Left-Multiplies the given matrix to the current matrix
    // (transformation is about the local origin of the object)
    STDMETHOD(MultMatrixLocal)(THIS_ CONST D3DXMATRIX* pM ) PURE;

    // Right multiply the current matrix with the computed rotation
    // matrix, counterclockwise about the given axis with the given angle.
    // (rotation is about the current world origin)
    STDMETHOD(RotateAxis)
        (THIS_ CONST D3DXVECTOR3* pV, FLOAT Angle) PURE;

    // Left multiply the current matrix with the computed rotation
    // matrix, counterclockwise about the given axis with the given angle.
    // (rotation is about the local origin of the object)
    STDMETHOD(RotateAxisLocal)
        (THIS_ CONST D3DXVECTOR3* pV, FLOAT Angle) PURE;

    // Right multiply the current matrix with the computed rotation
    // matrix. All angles are counterclockwise. (rotation is about the
    // current world origin)

    // The rotation is composed of a yaw around the Y axis, a pitch around
    // the X axis, and a roll around the Z axis.
    STDMETHOD(RotateYawPitchRoll)
        (THIS_ FLOAT Yaw, FLOAT Pitch, FLOAT Roll) PURE;

    // Left multiply the current matrix with the computed rotation
    // matrix. All angles are counterclockwise. (rotation is about the
    // local origin of the object)

    // The rotation is composed of a yaw around the Y axis, a pitch around
    // the X axis, and a roll around the Z axis.
    STDMETHOD(RotateYawPitchRollLocal)
        (THIS_ FLOAT Yaw, FLOAT Pitch, FLOAT Roll) PURE;

    // Right multiply the current matrix with the computed scale
    // matrix. (transformation is about the current world origin)
    STDMETHOD(Scale)(THIS_ FLOAT x, FLOAT y, FLOAT z) PURE;

    // Left multiply the current matrix with the computed scale
    // matrix. (transformation is about the local origin of the object)
    STDMETHOD(ScaleLocal)(THIS_ FLOAT x, FLOAT y, FLOAT z) PURE;

    // Right multiply the current matrix with the computed translation
    // matrix. (transformation is about the current world origin)
    STDMETHOD(Translate)(THIS_ FLOAT x, FLOAT y, FLOAT z ) PURE;

    // Left multiply the current matrix with the computed translation
    // matrix. (transformation is about the local origin of the object)
    STDMETHOD(TranslateLocal)(THIS_ FLOAT x, FLOAT y, FLOAT z) PURE;

    // Obtain the current matrix at the top of the stack
    STDMETHOD_(D3DXMATRIX*, GetTop)(THIS) PURE;
};

#ifdef __cplusplus
extern "C" {
#endif

HRESULT WINAPI 
    D3DXCreateMatrixStack( 
        DWORD               Flags, 
        LPD3DXMATRIXSTACK*  ppStack);

#ifdef __cplusplus
}
#endif

//===========================================================================
//
//  Spherical Harmonic Runtime Routines
//
// NOTE:
//  * Most of these functions can take the same object as in and out parameters.
//    The exceptions are the rotation functions.  
//
//  * Out parameters are typically also returned as return values, so that
//    the output of one function may be used as a parameter to another.
//
//============================================================================


// non-inline
#ifdef __cplusplus
extern "C" {
#endif

//============================================================================
//
//  Basic Spherical Harmonic math routines
//
//============================================================================

#define D3DXSH_MINORDER 2
#define D3DXSH_MAXORDER 6

//============================================================================
//
//  D3DXSHEvalDirection:
//  --------------------
//  Evaluates the Spherical Harmonic basis functions
//
//  Parameters:
//   pOut
//      Output SH coefficients - basis function Ylm is stored at l*l + m+l
//      This is the pointer that is returned.
//   Order
//      Order of the SH evaluation, generates Order^2 coefs, degree is Order-1
//   pDir
//      Direction to evaluate in - assumed to be normalized
//
//============================================================================

FLOAT* WINAPI D3DXSHEvalDirection
    (  FLOAT *pOut, UINT Order, CONST D3DXVECTOR3 *pDir );
    
//============================================================================
//
//  D3DXSHRotate:
//  --------------------
//  Rotates SH vector by a rotation matrix
//
//  Parameters:
//   pOut
//      Output SH coefficients - basis function Ylm is stored at l*l + m+l
//      This is the pointer that is returned (should not alias with pIn.)
//   Order
//      Order of the SH evaluation, generates Order^2 coefs, degree is Order-1
//   pMatrix
//      Matrix used for rotation - rotation sub matrix should be orthogonal
//      and have a unit determinant.
//   pIn
//      Input SH coeffs (rotated), incorect results if this is also output.
//
//============================================================================

FLOAT* WINAPI D3DXSHRotate
    ( FLOAT *pOut, UINT Order, CONST D3DXMATRIX *pMatrix, CONST FLOAT *pIn );
    
//============================================================================
//
//  D3DXSHRotateZ:
//  --------------------
//  Rotates the SH vector in the Z axis by an angle
//
//  Parameters:
//   pOut
//      Output SH coefficients - basis function Ylm is stored at l*l + m+l
//      This is the pointer that is returned (should not alias with pIn.)
//   Order
//      Order of the SH evaluation, generates Order^2 coefs, degree is Order-1
//   Angle
//      Angle in radians to rotate around the Z axis.
//   pIn
//      Input SH coeffs (rotated), incorect results if this is also output.
//
//============================================================================


FLOAT* WINAPI D3DXSHRotateZ
    ( FLOAT *pOut, UINT Order, FLOAT Angle, CONST FLOAT *pIn );
    
//============================================================================
//
//  D3DXSHAdd:
//  --------------------
//  Adds two SH vectors, pOut[i] = pA[i] + pB[i];
//
//  Parameters:
//   pOut
//      Output SH coefficients - basis function Ylm is stored at l*l + m+l
//      This is the pointer that is returned.
//   Order
//      Order of the SH evaluation, generates Order^2 coefs, degree is Order-1
//   pA
//      Input SH coeffs.
//   pB
//      Input SH coeffs (second vector.)
//
//============================================================================

FLOAT* WINAPI D3DXSHAdd
    ( FLOAT *pOut, UINT Order, CONST FLOAT *pA, CONST FLOAT *pB );

//============================================================================
//
//  D3DXSHScale:
//  --------------------
//  Adds two SH vectors, pOut[i] = pA[i]*Scale;
//
//  Parameters:
//   pOut
//      Output SH coefficients - basis function Ylm is stored at l*l + m+l
//      This is the pointer that is returned.
//   Order
//      Order of the SH evaluation, generates Order^2 coefs, degree is Order-1
//   pIn
//      Input SH coeffs.
//   Scale
//      Scale factor.
//
//============================================================================

FLOAT* WINAPI D3DXSHScale
    ( FLOAT *pOut, UINT Order, CONST FLOAT *pIn, CONST FLOAT Scale );
    
//============================================================================
//
//  D3DXSHDot:
//  --------------------
//  Computes the dot product of two SH vectors
//
//  Parameters:
//   Order
//      Order of the SH evaluation, generates Order^2 coefs, degree is Order-1
//   pA
//      Input SH coeffs.
//   pB
//      Second set of input SH coeffs.
//
//============================================================================

FLOAT WINAPI D3DXSHDot
    ( UINT Order, CONST FLOAT *pA, CONST FLOAT *pB );

//============================================================================
//
//  D3DXSHMultiply[O]:
//  --------------------
//  Computes the product of two functions represented using SH (f and g), where:
//  pOut[i] = int(y_i(s) * f(s) * g(s)), where y_i(s) is the ith SH basis
//  function, f(s) and g(s) are SH functions (sum_i(y_i(s)*c_i)).  The order O
//  determines the lengths of the arrays, where there should always be O^2 
//  coefficients.  In general the product of two SH functions of order O generates
//  and SH function of order 2*O - 1, but we truncate the result.  This means
//  that the product commutes (f*g == g*f) but doesn't associate 
//  (f*(g*h) != (f*g)*h.
//
//  Parameters:
//   pOut
//      Output SH coefficients - basis function Ylm is stored at l*l + m+l
//      This is the pointer that is returned.
//   pF
//      Input SH coeffs for first function.
//   pG
//      Second set of input SH coeffs.
//
//============================================================================

FLOAT* WINAPI D3DXSHMultiply2( FLOAT *pOut, CONST FLOAT *pF, CONST FLOAT *pG);
FLOAT* WINAPI D3DXSHMultiply3( FLOAT *pOut, CONST FLOAT *pF, CONST FLOAT *pG);
FLOAT* WINAPI D3DXSHMultiply4( FLOAT *pOut, CONST FLOAT *pF, CONST FLOAT *pG);
FLOAT* WINAPI D3DXSHMultiply5( FLOAT *pOut, CONST FLOAT *pF, CONST FLOAT *pG);
FLOAT* WINAPI D3DXSHMultiply6( FLOAT *pOut, CONST FLOAT *pF, CONST FLOAT *pG);


//============================================================================
//
//  Basic Spherical Harmonic lighting routines
//
//============================================================================

//============================================================================
//
//  D3DXSHEvalDirectionalLight:
//  --------------------
//  Evaluates a directional light and returns spectral SH data.  The output 
//  vector is computed so that if the intensity of R/G/B is unit the resulting
//  exit radiance of a point directly under the light on a diffuse object with
//  an albedo of 1 would be 1.0.  This will compute 3 spectral samples, pROut
//  has to be specified, while pGout and pBout are optional.
//
//  Parameters:
//   Order
//      Order of the SH evaluation, generates Order^2 coefs, degree is Order-1
//   pDir
//      Direction light is coming from (assumed to be normalized.)
//   RIntensity
//      Red intensity of light.
//   GIntensity
//      Green intensity of light.
//   BIntensity
//      Blue intensity of light.
//   pROut
//      Output SH vector for Red.
//   pGOut
//      Output SH vector for Green (optional.)
//   pBOut
//      Output SH vector for Blue (optional.)        
//
//============================================================================

HRESULT WINAPI D3DXSHEvalDirectionalLight
    ( UINT Order, CONST D3DXVECTOR3 *pDir, 
      FLOAT RIntensity, FLOAT GIntensity, FLOAT BIntensity,
      FLOAT *pROut, FLOAT *pGOut, FLOAT *pBOut );

//============================================================================
//
//  D3DXSHEvalSphericalLight:
//  --------------------
//  Evaluates a spherical light and returns spectral SH data.  There is no 
//  normalization of the intensity of the light like there is for directional
//  lights, care has to be taken when specifiying the intensities.  This will 
//  compute 3 spectral samples, pROut has to be specified, while pGout and 
//  pBout are optional.
//
//  Parameters:
//   Order
//      Order of the SH evaluation, generates Order^2 coefs, degree is Order-1
//   pPos
//      Position of light - reciever is assumed to be at the origin.
//   Radius
//      Radius of the spherical light source.
//   RIntensity
//      Red intensity of light.
//   GIntensity
//      Green intensity of light.
//   BIntensity
//      Blue intensity of light.
//   pROut
//      Output SH vector for Red.
//   pGOut
//      Output SH vector for Green (optional.)
//   pBOut
//      Output SH vector for Blue (optional.)        
//
//============================================================================

HRESULT WINAPI D3DXSHEvalSphericalLight
    ( UINT Order, CONST D3DXVECTOR3 *pPos, FLOAT Radius,
      FLOAT RIntensity, FLOAT GIntensity, FLOAT BIntensity,
      FLOAT *pROut, FLOAT *pGOut, FLOAT *pBOut );

//============================================================================
//
//  D3DXSHEvalConeLight:
//  --------------------
//  Evaluates a light that is a cone of constant intensity and returns spectral
//  SH data.  The output vector is computed so that if the intensity of R/G/B is
//  unit the resulting exit radiance of a point directly under the light oriented
//  in the cone direction on a diffuse object with an albedo of 1 would be 1.0.
//  This will compute 3 spectral samples, pROut has to be specified, while pGout
//  and pBout are optional.
//
//  Parameters:
//   Order
//      Order of the SH evaluation, generates Order^2 coefs, degree is Order-1
//   pDir
//      Direction light is coming from (assumed to be normalized.)
//   Radius
//      Radius of cone in radians.
//   RIntensity
//      Red intensity of light.
//   GIntensity
//      Green intensity of light.
//   BIntensity
//      Blue intensity of light.
//   pROut
//      Output SH vector for Red.
//   pGOut
//      Output SH vector for Green (optional.)
//   pBOut
//      Output SH vector for Blue (optional.)        
//
//============================================================================

HRESULT WINAPI D3DXSHEvalConeLight
    ( UINT Order, CONST D3DXVECTOR3 *pDir, FLOAT Radius,
      FLOAT RIntensity, FLOAT GIntensity, FLOAT BIntensity,
      FLOAT *pROut, FLOAT *pGOut, FLOAT *pBOut );
      
//============================================================================
//
//  D3DXSHEvalHemisphereLight:
//  --------------------
//  Evaluates a light that is a linear interpolant between two colors over the
//  sphere.  The interpolant is linear along the axis of the two points, not
//  over the surface of the sphere (ie: if the axis was (0,0,1) it is linear in
//  Z, not in the azimuthal angle.)  The resulting spherical lighting function
//  is normalized so that a point on a perfectly diffuse surface with no
//  shadowing and a normal pointed in the direction pDir would result in exit
//  radiance with a value of 1 if the top color was white and the bottom color
//  was black.  This is a very simple model where Top represents the intensity 
//  of the "sky" and Bottom represents the intensity of the "ground".
//
//  Parameters:
//   Order
//      Order of the SH evaluation, generates Order^2 coefs, degree is Order-1
//   pDir
//      Axis of the hemisphere.
//   Top
//      Color of the upper hemisphere.
//   Bottom
//      Color of the lower hemisphere.
//   pROut
//      Output SH vector for Red.
//   pGOut
//      Output SH vector for Green
//   pBOut
//      Output SH vector for Blue        
//
//============================================================================

HRESULT WINAPI D3DXSHEvalHemisphereLight
    ( UINT Order, CONST D3DXVECTOR3 *pDir, D3DXCOLOR Top, D3DXCOLOR Bottom,
      FLOAT *pROut, FLOAT *pGOut, FLOAT *pBOut );

//============================================================================
//
//  Basic Spherical Harmonic projection routines
//
//============================================================================

//============================================================================
//
//  D3DXSHProjectCubeMap:
//  --------------------
//  Projects a function represented on a cube map into spherical harmonics.
//
//  Parameters:
//   Order
//      Order of the SH evaluation, generates Order^2 coefs, degree is Order-1
//   pCubeMap
//      CubeMap that is going to be projected into spherical harmonics
//   pROut
//      Output SH vector for Red.
//   pGOut
//      Output SH vector for Green
//   pBOut
//      Output SH vector for Blue        
//
//============================================================================

HRESULT WINAPI D3DXSHProjectCubeMap
    ( UINT uOrder, LPDIRECT3DCUBETEXTURE9 pCubeMap,
      FLOAT *pROut, FLOAT *pGOut, FLOAT *pBOut );


#ifdef __cplusplus
}
#endif


//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) Microsoft Corporation.  All Rights Reserved.
//
//  File:       d3dx9math.inl
//  Content:    D3DX math inline functions
//
//////////////////////////////////////////////////////////////////////////////

#ifndef __D3DX9MATH_INL__
#define __D3DX9MATH_INL__

//===========================================================================
//
// Inline Class Methods
//
//===========================================================================

#ifdef __cplusplus

//--------------------------
// Float16
//--------------------------

D3DXINLINE
D3DXFLOAT16::D3DXFLOAT16( FLOAT f )
{
    D3DXFloat32To16Array(this, &f, 1);
}

D3DXINLINE
D3DXFLOAT16::D3DXFLOAT16( CONST D3DXFLOAT16& f )
{
    value = f.value;
}

// casting
D3DXINLINE
D3DXFLOAT16::operator FLOAT ()
{
    FLOAT f;
    D3DXFloat16To32Array(&f, this, 1);
    return f;
}

// binary operators
D3DXINLINE BOOL
D3DXFLOAT16::operator == ( CONST D3DXFLOAT16& f ) const
{
    return value == f.value;
}

D3DXINLINE BOOL
D3DXFLOAT16::operator != ( CONST D3DXFLOAT16& f ) const
{
    return value != f.value;
}


//--------------------------
// 2D Vector
//--------------------------

D3DXINLINE
D3DXVECTOR2::D3DXVECTOR2( CONST FLOAT *pf )
{
#ifdef D3DX_DEBUG
    if(!pf)
        return;
#endif

    x = pf[0];
    y = pf[1];
}

D3DXINLINE
D3DXVECTOR2::D3DXVECTOR2( CONST D3DXFLOAT16 *pf )
{
#ifdef D3DX_DEBUG
    if(!pf)
        return;
#endif

    D3DXFloat16To32Array(&x, pf, 2);    
}

D3DXINLINE
D3DXVECTOR2::D3DXVECTOR2( FLOAT fx, FLOAT fy )
{
    x = fx;
    y = fy;
}


// casting
D3DXINLINE
D3DXVECTOR2::operator FLOAT* ()
{
    return (FLOAT *) &x;
}

D3DXINLINE
D3DXVECTOR2::operator CONST FLOAT* () const
{
    return (CONST FLOAT *) &x;
}


// assignment operators
D3DXINLINE D3DXVECTOR2&
D3DXVECTOR2::operator += ( CONST D3DXVECTOR2& v )
{
    x += v.x;
    y += v.y;
    return *this;
}

D3DXINLINE D3DXVECTOR2&
D3DXVECTOR2::operator -= ( CONST D3DXVECTOR2& v )
{
    x -= v.x;
    y -= v.y;
    return *this;
}

D3DXINLINE D3DXVECTOR2&
D3DXVECTOR2::operator *= ( FLOAT f )
{
    x *= f;
    y *= f;
    return *this;
}

D3DXINLINE D3DXVECTOR2&
D3DXVECTOR2::operator /= ( FLOAT f )
{
    FLOAT fInv = 1.0f / f;
    x *= fInv;
    y *= fInv;
    return *this;
}


// unary operators
D3DXINLINE D3DXVECTOR2
D3DXVECTOR2::operator + () const
{
    return *this;
}

D3DXINLINE D3DXVECTOR2
D3DXVECTOR2::operator - () const
{
    return D3DXVECTOR2(-x, -y);
}


// binary operators
D3DXINLINE D3DXVECTOR2
D3DXVECTOR2::operator + ( CONST D3DXVECTOR2& v ) const
{
    return D3DXVECTOR2(x + v.x, y + v.y);
}

D3DXINLINE D3DXVECTOR2
D3DXVECTOR2::operator - ( CONST D3DXVECTOR2& v ) const
{
    return D3DXVECTOR2(x - v.x, y - v.y);
}

D3DXINLINE D3DXVECTOR2
D3DXVECTOR2::operator * ( FLOAT f ) const
{
    return D3DXVECTOR2(x * f, y * f);
}

D3DXINLINE D3DXVECTOR2
D3DXVECTOR2::operator / ( FLOAT f ) const
{
    FLOAT fInv = 1.0f / f;
    return D3DXVECTOR2(x * fInv, y * fInv);
}

D3DXINLINE D3DXVECTOR2
operator * ( FLOAT f, CONST D3DXVECTOR2& v )
{
    return D3DXVECTOR2(f * v.x, f * v.y);
}

D3DXINLINE BOOL
D3DXVECTOR2::operator == ( CONST D3DXVECTOR2& v ) const
{
    return x == v.x && y == v.y;
}

D3DXINLINE BOOL
D3DXVECTOR2::operator != ( CONST D3DXVECTOR2& v ) const
{
    return x != v.x || y != v.y;
}



//--------------------------
// 2D Vector (16 bit)
//--------------------------

D3DXINLINE
D3DXVECTOR2_16F::D3DXVECTOR2_16F( CONST FLOAT *pf )
{
#ifdef D3DX_DEBUG
    if(!pf)
        return;
#endif

    D3DXFloat32To16Array(&x, pf, 2);
}

D3DXINLINE
D3DXVECTOR2_16F::D3DXVECTOR2_16F( CONST D3DXFLOAT16 *pf )
{
#ifdef D3DX_DEBUG
    if(!pf)
        return;
#endif

    *((DWORD *) &x) = *((DWORD *) &pf[0]);
}

D3DXINLINE
D3DXVECTOR2_16F::D3DXVECTOR2_16F( CONST D3DXFLOAT16 &fx, CONST D3DXFLOAT16 &fy )
{
    x = fx;
    y = fy;
}


// casting
D3DXINLINE
D3DXVECTOR2_16F::operator D3DXFLOAT16* ()
{
    return (D3DXFLOAT16*) &x;
}

D3DXINLINE
D3DXVECTOR2_16F::operator CONST D3DXFLOAT16* () const
{
    return (CONST D3DXFLOAT16*) &x;
}


// binary operators
D3DXINLINE BOOL 
D3DXVECTOR2_16F::operator == ( CONST D3DXVECTOR2_16F &v ) const
{
    return *((DWORD *) &x) == *((DWORD *) &v.x);
}

D3DXINLINE BOOL 
D3DXVECTOR2_16F::operator != ( CONST D3DXVECTOR2_16F &v ) const
{
    return *((DWORD *) &x) != *((DWORD *) &v.x);
}


//--------------------------
// 3D Vector
//--------------------------
D3DXINLINE
D3DXVECTOR3::D3DXVECTOR3( CONST FLOAT *pf )
{
#ifdef D3DX_DEBUG
    if(!pf)
        return;
#endif

    x = pf[0];
    y = pf[1];
    z = pf[2];
}

D3DXINLINE
D3DXVECTOR3::D3DXVECTOR3( CONST D3DVECTOR& v )
{
    x = v.x;
    y = v.y;
    z = v.z;
}

D3DXINLINE
D3DXVECTOR3::D3DXVECTOR3( CONST D3DXFLOAT16 *pf )
{
#ifdef D3DX_DEBUG
    if(!pf)
        return;
#endif

    D3DXFloat16To32Array(&x, pf, 3);
}

D3DXINLINE
D3DXVECTOR3::D3DXVECTOR3( FLOAT fx, FLOAT fy, FLOAT fz )
{
    x = fx;
    y = fy;
    z = fz;
}


// casting
D3DXINLINE
D3DXVECTOR3::operator FLOAT* ()
{
    return (FLOAT *) &x;
}

D3DXINLINE
D3DXVECTOR3::operator CONST FLOAT* () const
{
    return (CONST FLOAT *) &x;
}


// assignment operators
D3DXINLINE D3DXVECTOR3&
D3DXVECTOR3::operator += ( CONST D3DXVECTOR3& v )
{
    x += v.x;
    y += v.y;
    z += v.z;
    return *this;
}

D3DXINLINE D3DXVECTOR3&
D3DXVECTOR3::operator -= ( CONST D3DXVECTOR3& v )
{
    x -= v.x;
    y -= v.y;
    z -= v.z;
    return *this;
}

D3DXINLINE D3DXVECTOR3&
D3DXVECTOR3::operator *= ( FLOAT f )
{
    x *= f;
    y *= f;
    z *= f;
    return *this;
}

D3DXINLINE D3DXVECTOR3&
D3DXVECTOR3::operator /= ( FLOAT f )
{
    FLOAT fInv = 1.0f / f;
    x *= fInv;
    y *= fInv;
    z *= fInv;
    return *this;
}


// unary operators
D3DXINLINE D3DXVECTOR3
D3DXVECTOR3::operator + () const
{
    return *this;
}

D3DXINLINE D3DXVECTOR3
D3DXVECTOR3::operator - () const
{
    return D3DXVECTOR3(-x, -y, -z);
}


// binary operators
D3DXINLINE D3DXVECTOR3
D3DXVECTOR3::operator + ( CONST D3DXVECTOR3& v ) const
{
    return D3DXVECTOR3(x + v.x, y + v.y, z + v.z);
}

D3DXINLINE D3DXVECTOR3
D3DXVECTOR3::operator - ( CONST D3DXVECTOR3& v ) const
{
    return D3DXVECTOR3(x - v.x, y - v.y, z - v.z);
}

D3DXINLINE D3DXVECTOR3
D3DXVECTOR3::operator * ( FLOAT f ) const
{
    return D3DXVECTOR3(x * f, y * f, z * f);
}

D3DXINLINE D3DXVECTOR3
D3DXVECTOR3::operator / ( FLOAT f ) const
{
    FLOAT fInv = 1.0f / f;
    return D3DXVECTOR3(x * fInv, y * fInv, z * fInv);
}


D3DXINLINE D3DXVECTOR3
operator * ( FLOAT f, CONST struct D3DXVECTOR3& v )
{
    return D3DXVECTOR3(f * v.x, f * v.y, f * v.z);
}


D3DXINLINE BOOL
D3DXVECTOR3::operator == ( CONST D3DXVECTOR3& v ) const
{
    return x == v.x && y == v.y && z == v.z;
}

D3DXINLINE BOOL
D3DXVECTOR3::operator != ( CONST D3DXVECTOR3& v ) const
{
    return x != v.x || y != v.y || z != v.z;
}



//--------------------------
// 3D Vector (16 bit)
//--------------------------

D3DXINLINE
D3DXVECTOR3_16F::D3DXVECTOR3_16F( CONST FLOAT *pf )
{
#ifdef D3DX_DEBUG
    if(!pf)
        return;
#endif

    D3DXFloat32To16Array(&x, pf, 3);
}

D3DXINLINE
D3DXVECTOR3_16F::D3DXVECTOR3_16F( CONST D3DVECTOR& v )
{
    D3DXFloat32To16Array(&x, &v.x, 1);
    D3DXFloat32To16Array(&y, &v.y, 1);
    D3DXFloat32To16Array(&z, &v.z, 1);
}

D3DXINLINE
D3DXVECTOR3_16F::D3DXVECTOR3_16F( CONST D3DXFLOAT16 *pf )
{
#ifdef D3DX_DEBUG
    if(!pf)
        return;
#endif

    *((DWORD *) &x) = *((DWORD *) &pf[0]);
    *((WORD  *) &z) = *((WORD  *) &pf[2]);
}

D3DXINLINE
D3DXVECTOR3_16F::D3DXVECTOR3_16F( CONST D3DXFLOAT16 &fx, CONST D3DXFLOAT16 &fy, CONST D3DXFLOAT16 &fz )
{
    x = fx;
    y = fy;
    z = fz;
}


// casting
D3DXINLINE
D3DXVECTOR3_16F::operator D3DXFLOAT16* ()
{
    return (D3DXFLOAT16*) &x;
}

D3DXINLINE
D3DXVECTOR3_16F::operator CONST D3DXFLOAT16* () const
{
    return (CONST D3DXFLOAT16*) &x;
}


// binary operators
D3DXINLINE BOOL 
D3DXVECTOR3_16F::operator == ( CONST D3DXVECTOR3_16F &v ) const
{
    return *((DWORD *) &x) == *((DWORD *) &v.x) &&
           *((WORD  *) &z) == *((WORD  *) &v.z);
}

D3DXINLINE BOOL 
D3DXVECTOR3_16F::operator != ( CONST D3DXVECTOR3_16F &v ) const
{
    return *((DWORD *) &x) != *((DWORD *) &v.x) ||
           *((WORD  *) &z) != *((WORD  *) &v.z);
}


//--------------------------
// 4D Vector
//--------------------------
D3DXINLINE
D3DXVECTOR4::D3DXVECTOR4( CONST FLOAT *pf )
{
#ifdef D3DX_DEBUG
    if(!pf)
        return;
#endif

    x = pf[0];
    y = pf[1];
    z = pf[2];
    w = pf[3];
}

D3DXINLINE
D3DXVECTOR4::D3DXVECTOR4( CONST D3DXFLOAT16 *pf )
{
#ifdef D3DX_DEBUG
    if(!pf)
        return;
#endif

    D3DXFloat16To32Array(&x, pf, 4);
}

D3DXINLINE
D3DXVECTOR4::D3DXVECTOR4( CONST D3DVECTOR& v, FLOAT f )
{
    x = v.x;
    y = v.y;
    z = v.z;
    w = f;
}

D3DXINLINE
D3DXVECTOR4::D3DXVECTOR4( FLOAT fx, FLOAT fy, FLOAT fz, FLOAT fw )
{
    x = fx;
    y = fy;
    z = fz;
    w = fw;
}


// casting
D3DXINLINE
D3DXVECTOR4::operator FLOAT* ()
{
    return (FLOAT *) &x;
}

D3DXINLINE
D3DXVECTOR4::operator CONST FLOAT* () const
{
    return (CONST FLOAT *) &x;
}


// assignment operators
D3DXINLINE D3DXVECTOR4&
D3DXVECTOR4::operator += ( CONST D3DXVECTOR4& v )
{
    x += v.x;
    y += v.y;
    z += v.z;
    w += v.w;
    return *this;
}

D3DXINLINE D3DXVECTOR4&
D3DXVECTOR4::operator -= ( CONST D3DXVECTOR4& v )
{
    x -= v.x;
    y -= v.y;
    z -= v.z;
    w -= v.w;
    return *this;
}

D3DXINLINE D3DXVECTOR4&
D3DXVECTOR4::operator *= ( FLOAT f )
{
    x *= f;
    y *= f;
    z *= f;
    w *= f;
    return *this;
}

D3DXINLINE D3DXVECTOR4&
D3DXVECTOR4::operator /= ( FLOAT f )
{
    FLOAT fInv = 1.0f / f;
    x *= fInv;
    y *= fInv;
    z *= fInv;
    w *= fInv;
    return *this;
}


// unary operators
D3DXINLINE D3DXVECTOR4
D3DXVECTOR4::operator + () const
{
    return *this;
}

D3DXINLINE D3DXVECTOR4
D3DXVECTOR4::operator - () const
{
    return D3DXVECTOR4(-x, -y, -z, -w);
}


// binary operators
D3DXINLINE D3DXVECTOR4
D3DXVECTOR4::operator + ( CONST D3DXVECTOR4& v ) const
{
    return D3DXVECTOR4(x + v.x, y + v.y, z + v.z, w + v.w);
}

D3DXINLINE D3DXVECTOR4
D3DXVECTOR4::operator - ( CONST D3DXVECTOR4& v ) const
{
    return D3DXVECTOR4(x - v.x, y - v.y, z - v.z, w - v.w);
}

D3DXINLINE D3DXVECTOR4
D3DXVECTOR4::operator * ( FLOAT f ) const
{
    return D3DXVECTOR4(x * f, y * f, z * f, w * f);
}

D3DXINLINE D3DXVECTOR4
D3DXVECTOR4::operator / ( FLOAT f ) const
{
    FLOAT fInv = 1.0f / f;
    return D3DXVECTOR4(x * fInv, y * fInv, z * fInv, w * fInv);
}

D3DXINLINE D3DXVECTOR4
operator * ( FLOAT f, CONST D3DXVECTOR4& v )
{
    return D3DXVECTOR4(f * v.x, f * v.y, f * v.z, f * v.w);
}


D3DXINLINE BOOL
D3DXVECTOR4::operator == ( CONST D3DXVECTOR4& v ) const
{
    return x == v.x && y == v.y && z == v.z && w == v.w;
}

D3DXINLINE BOOL
D3DXVECTOR4::operator != ( CONST D3DXVECTOR4& v ) const
{
    return x != v.x || y != v.y || z != v.z || w != v.w;
}



//--------------------------
// 4D Vector (16 bit)
//--------------------------

D3DXINLINE
D3DXVECTOR4_16F::D3DXVECTOR4_16F( CONST FLOAT *pf )
{
#ifdef D3DX_DEBUG
    if(!pf)
        return;
#endif

    D3DXFloat32To16Array(&x, pf, 4);
}

D3DXINLINE
D3DXVECTOR4_16F::D3DXVECTOR4_16F( CONST D3DXFLOAT16 *pf )
{
#ifdef D3DX_DEBUG
    if(!pf)
        return;
#endif

    *((DWORD *) &x) = *((DWORD *) &pf[0]);
    *((DWORD *) &z) = *((DWORD *) &pf[2]);
}

D3DXINLINE
D3DXVECTOR4_16F::D3DXVECTOR4_16F( CONST D3DXVECTOR3_16F& v, CONST D3DXFLOAT16& f )
{
    x = v.x;
    y = v.y;
    z = v.z;
    w = f;
}

D3DXINLINE
D3DXVECTOR4_16F::D3DXVECTOR4_16F( CONST D3DXFLOAT16 &fx, CONST D3DXFLOAT16 &fy, CONST D3DXFLOAT16 &fz, CONST D3DXFLOAT16 &fw )
{
    x = fx;
    y = fy;
    z = fz;
    w = fw;
}


// casting
D3DXINLINE
D3DXVECTOR4_16F::operator D3DXFLOAT16* ()
{
    return (D3DXFLOAT16*) &x;
}

D3DXINLINE
D3DXVECTOR4_16F::operator CONST D3DXFLOAT16* () const
{
    return (CONST D3DXFLOAT16*) &x;
}


// binary operators
D3DXINLINE BOOL 
D3DXVECTOR4_16F::operator == ( CONST D3DXVECTOR4_16F &v ) const
{
    return *((DWORD *) &x) == *((DWORD *) &v.x) &&
           *((DWORD *) &z) == *((DWORD *) &v.z);
}

D3DXINLINE BOOL 
D3DXVECTOR4_16F::operator != ( CONST D3DXVECTOR4_16F &v ) const
{
    return *((DWORD *) &x) != *((DWORD *) &v.x) ||
           *((DWORD *) &z) != *((DWORD *) &v.z);
}


//--------------------------
// Matrix
//--------------------------
D3DXINLINE
D3DXMATRIX::D3DXMATRIX( CONST FLOAT* pf )
{
#ifdef D3DX_DEBUG
    if(!pf)
        return;
#endif

    memcpy(&_11, pf, sizeof(D3DXMATRIX));
}

D3DXINLINE
D3DXMATRIX::D3DXMATRIX( CONST D3DMATRIX& mat )
{
    memcpy(&_11, &mat, sizeof(D3DXMATRIX));
}

D3DXINLINE
D3DXMATRIX::D3DXMATRIX( CONST D3DXFLOAT16* pf )
{
#ifdef D3DX_DEBUG
    if(!pf)
        return;
#endif

    D3DXFloat16To32Array(&_11, pf, 16);
}

D3DXINLINE
D3DXMATRIX::D3DXMATRIX( FLOAT f11, FLOAT f12, FLOAT f13, FLOAT f14,
                        FLOAT f21, FLOAT f22, FLOAT f23, FLOAT f24,
                        FLOAT f31, FLOAT f32, FLOAT f33, FLOAT f34,
                        FLOAT f41, FLOAT f42, FLOAT f43, FLOAT f44 )
{
    _11 = f11; _12 = f12; _13 = f13; _14 = f14;
    _21 = f21; _22 = f22; _23 = f23; _24 = f24;
    _31 = f31; _32 = f32; _33 = f33; _34 = f34;
    _41 = f41; _42 = f42; _43 = f43; _44 = f44;
}



// access grants
D3DXINLINE FLOAT&
D3DXMATRIX::operator () ( UINT iRow, UINT iCol )
{
    return m[iRow][iCol];
}

D3DXINLINE FLOAT
D3DXMATRIX::operator () ( UINT iRow, UINT iCol ) const
{
    return m[iRow][iCol];
}


// casting operators
D3DXINLINE
D3DXMATRIX::operator FLOAT* ()
{
    return (FLOAT *) &_11;
}

D3DXINLINE
D3DXMATRIX::operator CONST FLOAT* () const
{
    return (CONST FLOAT *) &_11;
}


// assignment operators
D3DXINLINE D3DXMATRIX&
D3DXMATRIX::operator *= ( CONST D3DXMATRIX& mat )
{
    D3DXMatrixMultiply(this, this, &mat);
    return *this;
}

D3DXINLINE D3DXMATRIX&
D3DXMATRIX::operator += ( CONST D3DXMATRIX& mat )
{
    _11 += mat._11; _12 += mat._12; _13 += mat._13; _14 += mat._14;
    _21 += mat._21; _22 += mat._22; _23 += mat._23; _24 += mat._24;
    _31 += mat._31; _32 += mat._32; _33 += mat._33; _34 += mat._34;
    _41 += mat._41; _42 += mat._42; _43 += mat._43; _44 += mat._44;
    return *this;
}

D3DXINLINE D3DXMATRIX&
D3DXMATRIX::operator -= ( CONST D3DXMATRIX& mat )
{
    _11 -= mat._11; _12 -= mat._12; _13 -= mat._13; _14 -= mat._14;
    _21 -= mat._21; _22 -= mat._22; _23 -= mat._23; _24 -= mat._24;
    _31 -= mat._31; _32 -= mat._32; _33 -= mat._33; _34 -= mat._34;
    _41 -= mat._41; _42 -= mat._42; _43 -= mat._43; _44 -= mat._44;
    return *this;
}

D3DXINLINE D3DXMATRIX&
D3DXMATRIX::operator *= ( FLOAT f )
{
    _11 *= f; _12 *= f; _13 *= f; _14 *= f;
    _21 *= f; _22 *= f; _23 *= f; _24 *= f;
    _31 *= f; _32 *= f; _33 *= f; _34 *= f;
    _41 *= f; _42 *= f; _43 *= f; _44 *= f;
    return *this;
}

D3DXINLINE D3DXMATRIX&
D3DXMATRIX::operator /= ( FLOAT f )
{
    FLOAT fInv = 1.0f / f;
    _11 *= fInv; _12 *= fInv; _13 *= fInv; _14 *= fInv;
    _21 *= fInv; _22 *= fInv; _23 *= fInv; _24 *= fInv;
    _31 *= fInv; _32 *= fInv; _33 *= fInv; _34 *= fInv;
    _41 *= fInv; _42 *= fInv; _43 *= fInv; _44 *= fInv;
    return *this;
}


// unary operators
D3DXINLINE D3DXMATRIX
D3DXMATRIX::operator + () const
{
    return *this;
}

D3DXINLINE D3DXMATRIX
D3DXMATRIX::operator - () const
{
    return D3DXMATRIX(-_11, -_12, -_13, -_14,
                      -_21, -_22, -_23, -_24,
                      -_31, -_32, -_33, -_34,
                      -_41, -_42, -_43, -_44);
}


// binary operators
D3DXINLINE D3DXMATRIX
D3DXMATRIX::operator * ( CONST D3DXMATRIX& mat ) const
{
    D3DXMATRIX matT;
    D3DXMatrixMultiply(&matT, this, &mat);
    return matT;
}

D3DXINLINE D3DXMATRIX
D3DXMATRIX::operator + ( CONST D3DXMATRIX& mat ) const
{
    return D3DXMATRIX(_11 + mat._11, _12 + mat._12, _13 + mat._13, _14 + mat._14,
                      _21 + mat._21, _22 + mat._22, _23 + mat._23, _24 + mat._24,
                      _31 + mat._31, _32 + mat._32, _33 + mat._33, _34 + mat._34,
                      _41 + mat._41, _42 + mat._42, _43 + mat._43, _44 + mat._44);
}

D3DXINLINE D3DXMATRIX
D3DXMATRIX::operator - ( CONST D3DXMATRIX& mat ) const
{
    return D3DXMATRIX(_11 - mat._11, _12 - mat._12, _13 - mat._13, _14 - mat._14,
                      _21 - mat._21, _22 - mat._22, _23 - mat._23, _24 - mat._24,
                      _31 - mat._31, _32 - mat._32, _33 - mat._33, _34 - mat._34,
                      _41 - mat._41, _42 - mat._42, _43 - mat._43, _44 - mat._44);
}

D3DXINLINE D3DXMATRIX
D3DXMATRIX::operator * ( FLOAT f ) const
{
    return D3DXMATRIX(_11 * f, _12 * f, _13 * f, _14 * f,
                      _21 * f, _22 * f, _23 * f, _24 * f,
                      _31 * f, _32 * f, _33 * f, _34 * f,
                      _41 * f, _42 * f, _43 * f, _44 * f);
}

D3DXINLINE D3DXMATRIX
D3DXMATRIX::operator / ( FLOAT f ) const
{
    FLOAT fInv = 1.0f / f;
    return D3DXMATRIX(_11 * fInv, _12 * fInv, _13 * fInv, _14 * fInv,
                      _21 * fInv, _22 * fInv, _23 * fInv, _24 * fInv,
                      _31 * fInv, _32 * fInv, _33 * fInv, _34 * fInv,
                      _41 * fInv, _42 * fInv, _43 * fInv, _44 * fInv);
}


D3DXINLINE D3DXMATRIX
operator * ( FLOAT f, CONST D3DXMATRIX& mat )
{
    return D3DXMATRIX(f * mat._11, f * mat._12, f * mat._13, f * mat._14,
                      f * mat._21, f * mat._22, f * mat._23, f * mat._24,
                      f * mat._31, f * mat._32, f * mat._33, f * mat._34,
                      f * mat._41, f * mat._42, f * mat._43, f * mat._44);
}


D3DXINLINE BOOL
D3DXMATRIX::operator == ( CONST D3DXMATRIX& mat ) const
{
    return 0 == memcmp(this, &mat, sizeof(D3DXMATRIX));
}

D3DXINLINE BOOL
D3DXMATRIX::operator != ( CONST D3DXMATRIX& mat ) const
{
    return 0 != memcmp(this, &mat, sizeof(D3DXMATRIX));
}



//--------------------------
// Aligned Matrices
//--------------------------

D3DXINLINE
_D3DXMATRIXA16::_D3DXMATRIXA16( CONST FLOAT* f ) : 
    D3DXMATRIX( f ) 
{
}

D3DXINLINE
_D3DXMATRIXA16::_D3DXMATRIXA16( CONST D3DMATRIX& m ) : 
    D3DXMATRIX( m ) 
{
}

D3DXINLINE
_D3DXMATRIXA16::_D3DXMATRIXA16( CONST D3DXFLOAT16* f ) : 
    D3DXMATRIX( f ) 
{
}

D3DXINLINE
_D3DXMATRIXA16::_D3DXMATRIXA16( FLOAT _11, FLOAT _12, FLOAT _13, FLOAT _14,
                                FLOAT _21, FLOAT _22, FLOAT _23, FLOAT _24,
                                FLOAT _31, FLOAT _32, FLOAT _33, FLOAT _34,
                                FLOAT _41, FLOAT _42, FLOAT _43, FLOAT _44 ) :
    D3DXMATRIX(_11, _12, _13, _14,
               _21, _22, _23, _24,
               _31, _32, _33, _34,
               _41, _42, _43, _44) 
{
}

#ifndef SIZE_MAX
#define SIZE_MAX ((SIZE_T)-1)
#endif

D3DXINLINE void* 
_D3DXMATRIXA16::operator new( size_t s )
{
    if (s > (SIZE_MAX-16))
	return NULL;
    LPBYTE p = ::new BYTE[s + 16];
    if (p)
    {
        BYTE offset = (BYTE)(16 - ((UINT_PTR)p & 15));
        p += offset;
        p[-1] = offset;
    }
    return p;
}

D3DXINLINE void* 
_D3DXMATRIXA16::operator new[]( size_t s )
{
    if (s > (SIZE_MAX-16))
	return NULL;
    LPBYTE p = ::new BYTE[s + 16];
    if (p)
    {
        BYTE offset = (BYTE)(16 - ((UINT_PTR)p & 15));
        p += offset;
        p[-1] = offset;
    }
    return p;
}

D3DXINLINE void 
_D3DXMATRIXA16::operator delete(void* p)
{
    if(p)
    {
        BYTE* pb = static_cast<BYTE*>(p);
        pb -= pb[-1];
        ::delete [] pb;
    }
}

D3DXINLINE void 
_D3DXMATRIXA16::operator delete[](void* p)
{
    if(p)
    {
        BYTE* pb = static_cast<BYTE*>(p);
        pb -= pb[-1];
        ::delete [] pb;
    }
}

D3DXINLINE _D3DXMATRIXA16& 
_D3DXMATRIXA16::operator=(CONST D3DXMATRIX& rhs)
{
    memcpy(&_11, &rhs, sizeof(D3DXMATRIX));
    return *this;
}


//--------------------------
// Quaternion
//--------------------------

D3DXINLINE
D3DXQUATERNION::D3DXQUATERNION( CONST FLOAT* pf )
{
#ifdef D3DX_DEBUG
    if(!pf)
        return;
#endif

    x = pf[0];
    y = pf[1];
    z = pf[2];
    w = pf[3];
}

D3DXINLINE
D3DXQUATERNION::D3DXQUATERNION( CONST D3DXFLOAT16* pf )
{
#ifdef D3DX_DEBUG
    if(!pf)
        return;
#endif

    D3DXFloat16To32Array(&x, pf, 4);
}

D3DXINLINE
D3DXQUATERNION::D3DXQUATERNION( FLOAT fx, FLOAT fy, FLOAT fz, FLOAT fw )
{
    x = fx;
    y = fy;
    z = fz;
    w = fw;
}


// casting
D3DXINLINE
D3DXQUATERNION::operator FLOAT* ()
{
    return (FLOAT *) &x;
}

D3DXINLINE
D3DXQUATERNION::operator CONST FLOAT* () const
{
    return (CONST FLOAT *) &x;
}


// assignment operators
D3DXINLINE D3DXQUATERNION&
D3DXQUATERNION::operator += ( CONST D3DXQUATERNION& q )
{
    x += q.x;
    y += q.y;
    z += q.z;
    w += q.w;
    return *this;
}

D3DXINLINE D3DXQUATERNION&
D3DXQUATERNION::operator -= ( CONST D3DXQUATERNION& q )
{
    x -= q.x;
    y -= q.y;
    z -= q.z;
    w -= q.w;
    return *this;
}

D3DXINLINE D3DXQUATERNION&
D3DXQUATERNION::operator *= ( CONST D3DXQUATERNION& q )
{
    D3DXQuaternionMultiply(this, this, &q);
    return *this;
}

D3DXINLINE D3DXQUATERNION&
D3DXQUATERNION::operator *= ( FLOAT f )
{
    x *= f;
    y *= f;
    z *= f;
    w *= f;
    return *this;
}

D3DXINLINE D3DXQUATERNION&
D3DXQUATERNION::operator /= ( FLOAT f )
{
    FLOAT fInv = 1.0f / f;
    x *= fInv;
    y *= fInv;
    z *= fInv;
    w *= fInv;
    return *this;
}


// unary operators
D3DXINLINE D3DXQUATERNION
D3DXQUATERNION::operator + () const
{
    return *this;
}

D3DXINLINE D3DXQUATERNION
D3DXQUATERNION::operator - () const
{
    return D3DXQUATERNION(-x, -y, -z, -w);
}


// binary operators
D3DXINLINE D3DXQUATERNION
D3DXQUATERNION::operator + ( CONST D3DXQUATERNION& q ) const
{
    return D3DXQUATERNION(x + q.x, y + q.y, z + q.z, w + q.w);
}

D3DXINLINE D3DXQUATERNION
D3DXQUATERNION::operator - ( CONST D3DXQUATERNION& q ) const
{
    return D3DXQUATERNION(x - q.x, y - q.y, z - q.z, w - q.w);
}

D3DXINLINE D3DXQUATERNION
D3DXQUATERNION::operator * ( CONST D3DXQUATERNION& q ) const
{
    D3DXQUATERNION qT;
    D3DXQuaternionMultiply(&qT, this, &q);
    return qT;
}

D3DXINLINE D3DXQUATERNION
D3DXQUATERNION::operator * ( FLOAT f ) const
{
    return D3DXQUATERNION(x * f, y * f, z * f, w * f);
}

D3DXINLINE D3DXQUATERNION
D3DXQUATERNION::operator / ( FLOAT f ) const
{
    FLOAT fInv = 1.0f / f;
    return D3DXQUATERNION(x * fInv, y * fInv, z * fInv, w * fInv);
}


D3DXINLINE D3DXQUATERNION
operator * (FLOAT f, CONST D3DXQUATERNION& q )
{
    return D3DXQUATERNION(f * q.x, f * q.y, f * q.z, f * q.w);
}


D3DXINLINE BOOL
D3DXQUATERNION::operator == ( CONST D3DXQUATERNION& q ) const
{
    return x == q.x && y == q.y && z == q.z && w == q.w;
}

D3DXINLINE BOOL
D3DXQUATERNION::operator != ( CONST D3DXQUATERNION& q ) const
{
    return x != q.x || y != q.y || z != q.z || w != q.w;
}



//--------------------------
// Plane
//--------------------------

D3DXINLINE
D3DXPLANE::D3DXPLANE( CONST FLOAT* pf )
{
#ifdef D3DX_DEBUG
    if(!pf)
        return;
#endif

    a = pf[0];
    b = pf[1];
    c = pf[2];
    d = pf[3];
}

D3DXINLINE
D3DXPLANE::D3DXPLANE( CONST D3DXFLOAT16* pf )
{
#ifdef D3DX_DEBUG
    if(!pf)
        return;
#endif

    D3DXFloat16To32Array(&a, pf, 4);
}

D3DXINLINE
D3DXPLANE::D3DXPLANE( FLOAT fa, FLOAT fb, FLOAT fc, FLOAT fd )
{
    a = fa;
    b = fb;
    c = fc;
    d = fd;
}


// casting
D3DXINLINE
D3DXPLANE::operator FLOAT* ()
{
    return (FLOAT *) &a;
}

D3DXINLINE
D3DXPLANE::operator CONST FLOAT* () const
{
    return (CONST FLOAT *) &a;
}


// assignment operators
D3DXINLINE D3DXPLANE&
D3DXPLANE::operator *= ( FLOAT f )
{
    a *= f;
    b *= f;
    c *= f;
    d *= f;
    return *this;
}

D3DXINLINE D3DXPLANE&
D3DXPLANE::operator /= ( FLOAT f )
{
    FLOAT fInv = 1.0f / f;
    a *= fInv;
    b *= fInv;
    c *= fInv;
    d *= fInv;
    return *this;
}


// unary operators
D3DXINLINE D3DXPLANE
D3DXPLANE::operator + () const
{
    return *this;
}

D3DXINLINE D3DXPLANE
D3DXPLANE::operator - () const
{
    return D3DXPLANE(-a, -b, -c, -d);
}


// binary operators
D3DXINLINE D3DXPLANE
D3DXPLANE::operator * ( FLOAT f ) const
{
    return D3DXPLANE(a * f, b * f, c * f, d * f);
}

D3DXINLINE D3DXPLANE
D3DXPLANE::operator / ( FLOAT f ) const
{
    FLOAT fInv = 1.0f / f;
    return D3DXPLANE(a * fInv, b * fInv, c * fInv, d * fInv);
}

D3DXINLINE D3DXPLANE
operator * (FLOAT f, CONST D3DXPLANE& p )
{
    return D3DXPLANE(f * p.a, f * p.b, f * p.c, f * p.d);
}

D3DXINLINE BOOL
D3DXPLANE::operator == ( CONST D3DXPLANE& p ) const
{
    return a == p.a && b == p.b && c == p.c && d == p.d;
}

D3DXINLINE BOOL
D3DXPLANE::operator != ( CONST D3DXPLANE& p ) const
{
    return a != p.a || b != p.b || c != p.c || d != p.d;
}




//--------------------------
// Color
//--------------------------

D3DXINLINE
D3DXCOLOR::D3DXCOLOR( DWORD dw )
{
   constexpr FLOAT f = 1.0f / 255.0f;
    r = f * (FLOAT) (unsigned char) (dw >> 16);
    g = f * (FLOAT) (unsigned char) (dw >>  8);
    b = f * (FLOAT) (unsigned char) (dw >>  0);
    a = f * (FLOAT) (unsigned char) (dw >> 24);
}

D3DXINLINE
D3DXCOLOR::D3DXCOLOR( CONST FLOAT* pf )
{
#ifdef D3DX_DEBUG
    if(!pf)
        return;
#endif

    r = pf[0];
    g = pf[1];
    b = pf[2];
    a = pf[3];
}

D3DXINLINE
D3DXCOLOR::D3DXCOLOR( CONST D3DXFLOAT16* pf )
{
#ifdef D3DX_DEBUG
    if(!pf)
        return;
#endif

    D3DXFloat16To32Array(&r, pf, 4);
}

D3DXINLINE
D3DXCOLOR::D3DXCOLOR( CONST D3DCOLORVALUE& c )
{
    r = c.r;
    g = c.g;
    b = c.b;
    a = c.a;
}

D3DXINLINE
D3DXCOLOR::D3DXCOLOR( FLOAT fr, FLOAT fg, FLOAT fb, FLOAT fa )
{
    r = fr;
    g = fg;
    b = fb;
    a = fa;
}


// casting
D3DXINLINE
D3DXCOLOR::operator DWORD () const
{
    DWORD dwR = r >= 1.0f ? 0xff : r <= 0.0f ? 0x00 : (DWORD) (r * 255.0f + 0.5f);
    DWORD dwG = g >= 1.0f ? 0xff : g <= 0.0f ? 0x00 : (DWORD) (g * 255.0f + 0.5f);
    DWORD dwB = b >= 1.0f ? 0xff : b <= 0.0f ? 0x00 : (DWORD) (b * 255.0f + 0.5f);
    DWORD dwA = a >= 1.0f ? 0xff : a <= 0.0f ? 0x00 : (DWORD) (a * 255.0f + 0.5f);

    return (dwA << 24) | (dwR << 16) | (dwG << 8) | dwB;
}


D3DXINLINE
D3DXCOLOR::operator FLOAT * ()
{
    return (FLOAT *) &r;
}

D3DXINLINE
D3DXCOLOR::operator CONST FLOAT * () const
{
    return (CONST FLOAT *) &r;
}


D3DXINLINE
D3DXCOLOR::operator D3DCOLORVALUE * ()
{
    return (D3DCOLORVALUE *) &r;
}

D3DXINLINE
D3DXCOLOR::operator CONST D3DCOLORVALUE * () const
{
    return (CONST D3DCOLORVALUE *) &r;
}


D3DXINLINE
D3DXCOLOR::operator D3DCOLORVALUE& ()
{
    return *((D3DCOLORVALUE *) &r);
}

D3DXINLINE
D3DXCOLOR::operator CONST D3DCOLORVALUE& () const
{
    return *((CONST D3DCOLORVALUE *) &r);
}


// assignment operators
D3DXINLINE D3DXCOLOR&
D3DXCOLOR::operator += ( CONST D3DXCOLOR& c )
{
    r += c.r;
    g += c.g;
    b += c.b;
    a += c.a;
    return *this;
}

D3DXINLINE D3DXCOLOR&
D3DXCOLOR::operator -= ( CONST D3DXCOLOR& c )
{
    r -= c.r;
    g -= c.g;
    b -= c.b;
    a -= c.a;
    return *this;
}

D3DXINLINE D3DXCOLOR&
D3DXCOLOR::operator *= ( FLOAT f )
{
    r *= f;
    g *= f;
    b *= f;
    a *= f;
    return *this;
}

D3DXINLINE D3DXCOLOR&
D3DXCOLOR::operator /= ( FLOAT f )
{
    FLOAT fInv = 1.0f / f;
    r *= fInv;
    g *= fInv;
    b *= fInv;
    a *= fInv;
    return *this;
}


// unary operators
D3DXINLINE D3DXCOLOR
D3DXCOLOR::operator + () const
{
    return *this;
}

D3DXINLINE D3DXCOLOR
D3DXCOLOR::operator - () const
{
    return D3DXCOLOR(-r, -g, -b, -a);
}


// binary operators
D3DXINLINE D3DXCOLOR
D3DXCOLOR::operator + ( CONST D3DXCOLOR& c ) const
{
    return D3DXCOLOR(r + c.r, g + c.g, b + c.b, a + c.a);
}

D3DXINLINE D3DXCOLOR
D3DXCOLOR::operator - ( CONST D3DXCOLOR& c ) const
{
    return D3DXCOLOR(r - c.r, g - c.g, b - c.b, a - c.a);
}

D3DXINLINE D3DXCOLOR
D3DXCOLOR::operator * ( FLOAT f ) const
{
    return D3DXCOLOR(r * f, g * f, b * f, a * f);
}

D3DXINLINE D3DXCOLOR
D3DXCOLOR::operator / ( FLOAT f ) const
{
    FLOAT fInv = 1.0f / f;
    return D3DXCOLOR(r * fInv, g * fInv, b * fInv, a * fInv);
}


D3DXINLINE D3DXCOLOR
operator * (FLOAT f, CONST D3DXCOLOR& c )
{
    return D3DXCOLOR(f * c.r, f * c.g, f * c.b, f * c.a);
}


D3DXINLINE BOOL
D3DXCOLOR::operator == ( CONST D3DXCOLOR& c ) const
{
    return r == c.r && g == c.g && b == c.b && a == c.a;
}

D3DXINLINE BOOL
D3DXCOLOR::operator != ( CONST D3DXCOLOR& c ) const
{
    return r != c.r || g != c.g || b != c.b || a != c.a;
}


#endif //__cplusplus



//===========================================================================
//
// Inline functions
//
//===========================================================================


//--------------------------
// 2D Vector
//--------------------------

D3DXINLINE FLOAT D3DXVec2Length
    ( CONST D3DXVECTOR2 *pV )
{
#ifdef D3DX_DEBUG
    if(!pV)
        return 0.0f;
#endif

#ifdef __cplusplus
    return sqrtf(pV->x * pV->x + pV->y * pV->y);
#else
    return (FLOAT) sqrt(pV->x * pV->x + pV->y * pV->y);
#endif
}

D3DXINLINE FLOAT D3DXVec2LengthSq
    ( CONST D3DXVECTOR2 *pV )
{
#ifdef D3DX_DEBUG
    if(!pV)
        return 0.0f;
#endif

    return pV->x * pV->x + pV->y * pV->y;
}

D3DXINLINE FLOAT D3DXVec2Dot
    ( CONST D3DXVECTOR2 *pV1, CONST D3DXVECTOR2 *pV2 )
{
#ifdef D3DX_DEBUG
    if(!pV1 || !pV2)
        return 0.0f;
#endif

    return pV1->x * pV2->x + pV1->y * pV2->y;
}

D3DXINLINE FLOAT D3DXVec2CCW
    ( CONST D3DXVECTOR2 *pV1, CONST D3DXVECTOR2 *pV2 )
{
#ifdef D3DX_DEBUG
    if(!pV1 || !pV2)
        return 0.0f;
#endif

    return pV1->x * pV2->y - pV1->y * pV2->x;
}

D3DXINLINE D3DXVECTOR2* D3DXVec2Add
    ( D3DXVECTOR2 *pOut, CONST D3DXVECTOR2 *pV1, CONST D3DXVECTOR2 *pV2 )
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV1 || !pV2)
        return NULL;
#endif

    pOut->x = pV1->x + pV2->x;
    pOut->y = pV1->y + pV2->y;
    return pOut;
}

D3DXINLINE D3DXVECTOR2* D3DXVec2Subtract
    ( D3DXVECTOR2 *pOut, CONST D3DXVECTOR2 *pV1, CONST D3DXVECTOR2 *pV2 )
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV1 || !pV2)
        return NULL;
#endif

    pOut->x = pV1->x - pV2->x;
    pOut->y = pV1->y - pV2->y;
    return pOut;
}

D3DXINLINE D3DXVECTOR2* D3DXVec2Minimize
    ( D3DXVECTOR2 *pOut, CONST D3DXVECTOR2 *pV1, CONST D3DXVECTOR2 *pV2 )
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV1 || !pV2)
        return NULL;
#endif

    pOut->x = pV1->x < pV2->x ? pV1->x : pV2->x;
    pOut->y = pV1->y < pV2->y ? pV1->y : pV2->y;
    return pOut;
}

D3DXINLINE D3DXVECTOR2* D3DXVec2Maximize
    ( D3DXVECTOR2 *pOut, CONST D3DXVECTOR2 *pV1, CONST D3DXVECTOR2 *pV2 )
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV1 || !pV2)
        return NULL;
#endif

    pOut->x = pV1->x > pV2->x ? pV1->x : pV2->x;
    pOut->y = pV1->y > pV2->y ? pV1->y : pV2->y;
    return pOut;
}

D3DXINLINE D3DXVECTOR2* D3DXVec2Scale
    ( D3DXVECTOR2 *pOut, CONST D3DXVECTOR2 *pV, FLOAT s )
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV)
        return NULL;
#endif

    pOut->x = pV->x * s;
    pOut->y = pV->y * s;
    return pOut;
}

D3DXINLINE D3DXVECTOR2* D3DXVec2Lerp
    ( D3DXVECTOR2 *pOut, CONST D3DXVECTOR2 *pV1, CONST D3DXVECTOR2 *pV2,
      FLOAT s )
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV1 || !pV2)
        return NULL;
#endif

    pOut->x = pV1->x + s * (pV2->x - pV1->x);
    pOut->y = pV1->y + s * (pV2->y - pV1->y);
    return pOut;
}


//--------------------------
// 3D Vector
//--------------------------

D3DXINLINE FLOAT D3DXVec3Length
    ( CONST D3DXVECTOR3 *pV )
{
#ifdef D3DX_DEBUG
    if(!pV)
        return 0.0f;
#endif

#ifdef __cplusplus
    return sqrtf(pV->x * pV->x + pV->y * pV->y + pV->z * pV->z);
#else
    return (FLOAT) sqrt(pV->x * pV->x + pV->y * pV->y + pV->z * pV->z);
#endif
}

D3DXINLINE FLOAT D3DXVec3LengthSq
    ( CONST D3DXVECTOR3 *pV )
{
#ifdef D3DX_DEBUG
    if(!pV)
        return 0.0f;
#endif

    return pV->x * pV->x + pV->y * pV->y + pV->z * pV->z;
}

D3DXINLINE FLOAT D3DXVec3Dot
    ( CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2 )
{
#ifdef D3DX_DEBUG
    if(!pV1 || !pV2)
        return 0.0f;
#endif

    return pV1->x * pV2->x + pV1->y * pV2->y + pV1->z * pV2->z;
}

D3DXINLINE D3DXVECTOR3* D3DXVec3Cross
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2 )
{
    D3DXVECTOR3 v;

#ifdef D3DX_DEBUG
    if(!pOut || !pV1 || !pV2)
        return NULL;
#endif

    v.x = pV1->y * pV2->z - pV1->z * pV2->y;
    v.y = pV1->z * pV2->x - pV1->x * pV2->z;
    v.z = pV1->x * pV2->y - pV1->y * pV2->x;

    *pOut = v;
    return pOut;
}

D3DXINLINE D3DXVECTOR3* D3DXVec3Add
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2 )
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV1 || !pV2)
        return NULL;
#endif

    pOut->x = pV1->x + pV2->x;
    pOut->y = pV1->y + pV2->y;
    pOut->z = pV1->z + pV2->z;
    return pOut;
}

D3DXINLINE D3DXVECTOR3* D3DXVec3Subtract
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2 )
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV1 || !pV2)
        return NULL;
#endif

    pOut->x = pV1->x - pV2->x;
    pOut->y = pV1->y - pV2->y;
    pOut->z = pV1->z - pV2->z;
    return pOut;
}

D3DXINLINE D3DXVECTOR3* D3DXVec3Minimize
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2 )
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV1 || !pV2)
        return NULL;
#endif

    pOut->x = pV1->x < pV2->x ? pV1->x : pV2->x;
    pOut->y = pV1->y < pV2->y ? pV1->y : pV2->y;
    pOut->z = pV1->z < pV2->z ? pV1->z : pV2->z;
    return pOut;
}

D3DXINLINE D3DXVECTOR3* D3DXVec3Maximize
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2 )
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV1 || !pV2)
        return NULL;
#endif

    pOut->x = pV1->x > pV2->x ? pV1->x : pV2->x;
    pOut->y = pV1->y > pV2->y ? pV1->y : pV2->y;
    pOut->z = pV1->z > pV2->z ? pV1->z : pV2->z;
    return pOut;
}

D3DXINLINE D3DXVECTOR3* D3DXVec3Scale
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV, FLOAT s)
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV)
        return NULL;
#endif

    pOut->x = pV->x * s;
    pOut->y = pV->y * s;
    pOut->z = pV->z * s;
    return pOut;
}

D3DXINLINE D3DXVECTOR3* D3DXVec3Lerp
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2,
      FLOAT s )
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV1 || !pV2)
        return NULL;
#endif

    pOut->x = pV1->x + s * (pV2->x - pV1->x);
    pOut->y = pV1->y + s * (pV2->y - pV1->y);
    pOut->z = pV1->z + s * (pV2->z - pV1->z);
    return pOut;
}


//--------------------------
// 4D Vector
//--------------------------

D3DXINLINE FLOAT D3DXVec4Length
    ( CONST D3DXVECTOR4 *pV )
{
#ifdef D3DX_DEBUG
    if(!pV)
        return 0.0f;
#endif

#ifdef __cplusplus
    return sqrtf(pV->x * pV->x + pV->y * pV->y + pV->z * pV->z + pV->w * pV->w);
#else
    return (FLOAT) sqrt(pV->x * pV->x + pV->y * pV->y + pV->z * pV->z + pV->w * pV->w);
#endif
}

D3DXINLINE FLOAT D3DXVec4LengthSq
    ( CONST D3DXVECTOR4 *pV )
{
#ifdef D3DX_DEBUG
    if(!pV)
        return 0.0f;
#endif

    return pV->x * pV->x + pV->y * pV->y + pV->z * pV->z + pV->w * pV->w;
}

D3DXINLINE FLOAT D3DXVec4Dot
    ( CONST D3DXVECTOR4 *pV1, CONST D3DXVECTOR4 *pV2 )
{
#ifdef D3DX_DEBUG
    if(!pV1 || !pV2)
        return 0.0f;
#endif

    return pV1->x * pV2->x + pV1->y * pV2->y + pV1->z * pV2->z + pV1->w * pV2->w;
}

D3DXINLINE D3DXVECTOR4* D3DXVec4Add
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV1, CONST D3DXVECTOR4 *pV2)
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV1 || !pV2)
        return NULL;
#endif

    pOut->x = pV1->x + pV2->x;
    pOut->y = pV1->y + pV2->y;
    pOut->z = pV1->z + pV2->z;
    pOut->w = pV1->w + pV2->w;
    return pOut;
}

D3DXINLINE D3DXVECTOR4* D3DXVec4Subtract
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV1, CONST D3DXVECTOR4 *pV2)
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV1 || !pV2)
        return NULL;
#endif

    pOut->x = pV1->x - pV2->x;
    pOut->y = pV1->y - pV2->y;
    pOut->z = pV1->z - pV2->z;
    pOut->w = pV1->w - pV2->w;
    return pOut;
}

D3DXINLINE D3DXVECTOR4* D3DXVec4Minimize
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV1, CONST D3DXVECTOR4 *pV2)
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV1 || !pV2)
        return NULL;
#endif

    pOut->x = pV1->x < pV2->x ? pV1->x : pV2->x;
    pOut->y = pV1->y < pV2->y ? pV1->y : pV2->y;
    pOut->z = pV1->z < pV2->z ? pV1->z : pV2->z;
    pOut->w = pV1->w < pV2->w ? pV1->w : pV2->w;
    return pOut;
}

D3DXINLINE D3DXVECTOR4* D3DXVec4Maximize
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV1, CONST D3DXVECTOR4 *pV2)
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV1 || !pV2)
        return NULL;
#endif

    pOut->x = pV1->x > pV2->x ? pV1->x : pV2->x;
    pOut->y = pV1->y > pV2->y ? pV1->y : pV2->y;
    pOut->z = pV1->z > pV2->z ? pV1->z : pV2->z;
    pOut->w = pV1->w > pV2->w ? pV1->w : pV2->w;
    return pOut;
}

D3DXINLINE D3DXVECTOR4* D3DXVec4Scale
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV, FLOAT s)
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV)
        return NULL;
#endif

    pOut->x = pV->x * s;
    pOut->y = pV->y * s;
    pOut->z = pV->z * s;
    pOut->w = pV->w * s;
    return pOut;
}

D3DXINLINE D3DXVECTOR4* D3DXVec4Lerp
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV1, CONST D3DXVECTOR4 *pV2,
      FLOAT s )
{
#ifdef D3DX_DEBUG
    if(!pOut || !pV1 || !pV2)
        return NULL;
#endif

    pOut->x = pV1->x + s * (pV2->x - pV1->x);
    pOut->y = pV1->y + s * (pV2->y - pV1->y);
    pOut->z = pV1->z + s * (pV2->z - pV1->z);
    pOut->w = pV1->w + s * (pV2->w - pV1->w);
    return pOut;
}


//--------------------------
// 4D Matrix
//--------------------------

D3DXINLINE D3DXMATRIX* D3DXMatrixIdentity
    ( D3DXMATRIX *pOut )
{
#ifdef D3DX_DEBUG
    if(!pOut)
        return NULL;
#endif

    pOut->m[0][1] = pOut->m[0][2] = pOut->m[0][3] =
    pOut->m[1][0] = pOut->m[1][2] = pOut->m[1][3] =
    pOut->m[2][0] = pOut->m[2][1] = pOut->m[2][3] =
    pOut->m[3][0] = pOut->m[3][1] = pOut->m[3][2] = 0.0f;

    pOut->m[0][0] = pOut->m[1][1] = pOut->m[2][2] = pOut->m[3][3] = 1.0f;
    return pOut;
}


D3DXINLINE BOOL D3DXMatrixIsIdentity
    ( CONST D3DXMATRIX *pM )
{
#ifdef D3DX_DEBUG
    if(!pM)
        return FALSE;
#endif

    return pM->m[0][0] == 1.0f && pM->m[0][1] == 0.0f && pM->m[0][2] == 0.0f && pM->m[0][3] == 0.0f &&
           pM->m[1][0] == 0.0f && pM->m[1][1] == 1.0f && pM->m[1][2] == 0.0f && pM->m[1][3] == 0.0f &&
           pM->m[2][0] == 0.0f && pM->m[2][1] == 0.0f && pM->m[2][2] == 1.0f && pM->m[2][3] == 0.0f &&
           pM->m[3][0] == 0.0f && pM->m[3][1] == 0.0f && pM->m[3][2] == 0.0f && pM->m[3][3] == 1.0f;
}


//--------------------------
// Quaternion
//--------------------------

D3DXINLINE FLOAT D3DXQuaternionLength
    ( CONST D3DXQUATERNION *pQ )
{
#ifdef D3DX_DEBUG
    if(!pQ)
        return 0.0f;
#endif

#ifdef __cplusplus
    return sqrtf(pQ->x * pQ->x + pQ->y * pQ->y + pQ->z * pQ->z + pQ->w * pQ->w);
#else
    return (FLOAT) sqrt(pQ->x * pQ->x + pQ->y * pQ->y + pQ->z * pQ->z + pQ->w * pQ->w);
#endif
}

D3DXINLINE FLOAT D3DXQuaternionLengthSq
    ( CONST D3DXQUATERNION *pQ )
{
#ifdef D3DX_DEBUG
    if(!pQ)
        return 0.0f;
#endif

    return pQ->x * pQ->x + pQ->y * pQ->y + pQ->z * pQ->z + pQ->w * pQ->w;
}

D3DXINLINE FLOAT D3DXQuaternionDot
    ( CONST D3DXQUATERNION *pQ1, CONST D3DXQUATERNION *pQ2 )
{
#ifdef D3DX_DEBUG
    if(!pQ1 || !pQ2)
        return 0.0f;
#endif

    return pQ1->x * pQ2->x + pQ1->y * pQ2->y + pQ1->z * pQ2->z + pQ1->w * pQ2->w;
}


D3DXINLINE D3DXQUATERNION* D3DXQuaternionIdentity
    ( D3DXQUATERNION *pOut )
{
#ifdef D3DX_DEBUG
    if(!pOut)
        return NULL;
#endif

    pOut->x = pOut->y = pOut->z = 0.0f;
    pOut->w = 1.0f;
    return pOut;
}

D3DXINLINE BOOL D3DXQuaternionIsIdentity
    ( CONST D3DXQUATERNION *pQ )
{
#ifdef D3DX_DEBUG
    if(!pQ)
        return FALSE;
#endif

    return pQ->x == 0.0f && pQ->y == 0.0f && pQ->z == 0.0f && pQ->w == 1.0f;
}


D3DXINLINE D3DXQUATERNION* D3DXQuaternionConjugate
    ( D3DXQUATERNION *pOut, CONST D3DXQUATERNION *pQ )
{
#ifdef D3DX_DEBUG
    if(!pOut || !pQ)
        return NULL;
#endif

    pOut->x = -pQ->x;
    pOut->y = -pQ->y;
    pOut->z = -pQ->z;
    pOut->w =  pQ->w;
    return pOut;
}


//--------------------------
// Plane
//--------------------------

D3DXINLINE FLOAT D3DXPlaneDot
    ( CONST D3DXPLANE *pP, CONST D3DXVECTOR4 *pV)
{
#ifdef D3DX_DEBUG
    if(!pP || !pV)
        return 0.0f;
#endif

    return pP->a * pV->x + pP->b * pV->y + pP->c * pV->z + pP->d * pV->w;
}

D3DXINLINE FLOAT D3DXPlaneDotCoord
    ( CONST D3DXPLANE *pP, CONST D3DXVECTOR3 *pV)
{
#ifdef D3DX_DEBUG
    if(!pP || !pV)
        return 0.0f;
#endif

    return pP->a * pV->x + pP->b * pV->y + pP->c * pV->z + pP->d;
}

D3DXINLINE FLOAT D3DXPlaneDotNormal
    ( CONST D3DXPLANE *pP, CONST D3DXVECTOR3 *pV)
{
#ifdef D3DX_DEBUG
    if(!pP || !pV)
        return 0.0f;
#endif

    return pP->a * pV->x + pP->b * pV->y + pP->c * pV->z;
}

D3DXINLINE D3DXPLANE* D3DXPlaneScale
    (D3DXPLANE *pOut, CONST D3DXPLANE *pP, FLOAT s)
{
#ifdef D3DX_DEBUG
    if(!pOut || !pP)
        return NULL;
#endif

    pOut->a = pP->a * s;
    pOut->b = pP->b * s;
    pOut->c = pP->c * s;
    pOut->d = pP->d * s;
    return pOut;
}


//--------------------------
// Color
//--------------------------

D3DXINLINE D3DXCOLOR* D3DXColorNegative
    (D3DXCOLOR *pOut, CONST D3DXCOLOR *pC)
{
#ifdef D3DX_DEBUG
    if(!pOut || !pC)
        return NULL;
#endif

    pOut->r = 1.0f - pC->r;
    pOut->g = 1.0f - pC->g;
    pOut->b = 1.0f - pC->b;
    pOut->a = pC->a;
    return pOut;
}

D3DXINLINE D3DXCOLOR* D3DXColorAdd
    (D3DXCOLOR *pOut, CONST D3DXCOLOR *pC1, CONST D3DXCOLOR *pC2)
{
#ifdef D3DX_DEBUG
    if(!pOut || !pC1 || !pC2)
        return NULL;
#endif

    pOut->r = pC1->r + pC2->r;
    pOut->g = pC1->g + pC2->g;
    pOut->b = pC1->b + pC2->b;
    pOut->a = pC1->a + pC2->a;
    return pOut;
}

D3DXINLINE D3DXCOLOR* D3DXColorSubtract
    (D3DXCOLOR *pOut, CONST D3DXCOLOR *pC1, CONST D3DXCOLOR *pC2)
{
#ifdef D3DX_DEBUG
    if(!pOut || !pC1 || !pC2)
        return NULL;
#endif

    pOut->r = pC1->r - pC2->r;
    pOut->g = pC1->g - pC2->g;
    pOut->b = pC1->b - pC2->b;
    pOut->a = pC1->a - pC2->a;
    return pOut;
}

D3DXINLINE D3DXCOLOR* D3DXColorScale
    (D3DXCOLOR *pOut, CONST D3DXCOLOR *pC, FLOAT s)
{
#ifdef D3DX_DEBUG
    if(!pOut || !pC)
        return NULL;
#endif

    pOut->r = pC->r * s;
    pOut->g = pC->g * s;
    pOut->b = pC->b * s;
    pOut->a = pC->a * s;
    return pOut;
}

D3DXINLINE D3DXCOLOR* D3DXColorModulate
    (D3DXCOLOR *pOut, CONST D3DXCOLOR *pC1, CONST D3DXCOLOR *pC2)
{
#ifdef D3DX_DEBUG
    if(!pOut || !pC1 || !pC2)
        return NULL;
#endif

    pOut->r = pC1->r * pC2->r;
    pOut->g = pC1->g * pC2->g;
    pOut->b = pC1->b * pC2->b;
    pOut->a = pC1->a * pC2->a;
    return pOut;
}

D3DXINLINE D3DXCOLOR* D3DXColorLerp
    (D3DXCOLOR *pOut, CONST D3DXCOLOR *pC1, CONST D3DXCOLOR *pC2, FLOAT s)
{
#ifdef D3DX_DEBUG
    if(!pOut || !pC1 || !pC2)
        return NULL;
#endif

    pOut->r = pC1->r + s * (pC2->r - pC1->r);
    pOut->g = pC1->g + s * (pC2->g - pC1->g);
    pOut->b = pC1->b + s * (pC2->b - pC1->b);
    pOut->a = pC1->a + s * (pC2->a - pC1->a);
    return pOut;
}


#endif // __D3DX9MATH_INL__



#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4201)
#endif

#endif // __D3DX9MATH_H__




// Errors
#define _FACDD  0x876
#define MAKE_DDHRESULT( code )  MAKE_HRESULT( 1, _FACDD, code )

enum _D3DXERR {
    D3DXERR_CANNOTMODIFYINDEXBUFFER     = MAKE_DDHRESULT(2900),
    D3DXERR_INVALIDMESH                 = MAKE_DDHRESULT(2901),
    D3DXERR_CANNOTATTRSORT              = MAKE_DDHRESULT(2902),
    D3DXERR_SKINNINGNOTSUPPORTED        = MAKE_DDHRESULT(2903),
    D3DXERR_TOOMANYINFLUENCES           = MAKE_DDHRESULT(2904),
    D3DXERR_INVALIDDATA                 = MAKE_DDHRESULT(2905),
    D3DXERR_LOADEDMESHASNODATA          = MAKE_DDHRESULT(2906),
    D3DXERR_DUPLICATENAMEDFRAGMENT      = MAKE_DDHRESULT(2907),
	D3DXERR_CANNOTREMOVELASTITEM		= MAKE_DDHRESULT(2908),
};


#endif //__D3DX9_H__


///////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) Microsoft Corporation.  All Rights Reserved.
//
//  File:       d3dx9core.h
//  Content:    D3DX core types and functions
//
///////////////////////////////////////////////////////////////////////////

#ifndef __D3DX9CORE_H__
#define __D3DX9CORE_H__


///////////////////////////////////////////////////////////////////////////
// D3DX_SDK_VERSION:
// -----------------
// This identifier is passed to D3DXCheckVersion in order to ensure that an
// application was built against the correct header files and lib files. 
// This number is incremented whenever a header (or other) change would 
// require applications to be rebuilt. If the version doesn't match, 
// D3DXCheckVersion will return FALSE. (The number itself has no meaning.)
///////////////////////////////////////////////////////////////////////////

#define D3DX_VERSION 0x0902

#define D3DX_SDK_VERSION 43

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

BOOL WINAPI
    D3DXCheckVersion(UINT D3DSdkVersion, UINT D3DXSdkVersion);

#ifdef __cplusplus
}
#endif //__cplusplus



///////////////////////////////////////////////////////////////////////////
// D3DXDebugMute
//    Mutes D3DX and D3D debug spew (TRUE - mute, FALSE - not mute)
//
//  returns previous mute value
//
///////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

BOOL WINAPI
    D3DXDebugMute(BOOL Mute);  

#ifdef __cplusplus
}
#endif //__cplusplus


///////////////////////////////////////////////////////////////////////////
// D3DXGetDriverLevel:
//    Returns driver version information:
//
//    700 - DX7 level driver
//    800 - DX8 level driver
//    900 - DX9 level driver
///////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

UINT WINAPI
    D3DXGetDriverLevel(LPDIRECT3DDEVICE9 pDevice);

#ifdef __cplusplus
}
#endif //__cplusplus


///////////////////////////////////////////////////////////////////////////
// ID3DXBuffer:
// ------------
// The buffer object is used by D3DX to return arbitrary size data.
//
// GetBufferPointer -
//    Returns a pointer to the beginning of the buffer.
//
// GetBufferSize -
//    Returns the size of the buffer, in bytes.
///////////////////////////////////////////////////////////////////////////

typedef interface ID3DXBuffer ID3DXBuffer;
typedef interface ID3DXBuffer *LPD3DXBUFFER;

// {8BA5FB08-5195-40e2-AC58-0D989C3A0102}
DEFINE_GUID(IID_ID3DXBuffer, 
0x8ba5fb08, 0x5195, 0x40e2, 0xac, 0x58, 0xd, 0x98, 0x9c, 0x3a, 0x1, 0x2);

#undef INTERFACE
#define INTERFACE ID3DXBuffer

DECLARE_INTERFACE_(ID3DXBuffer, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // ID3DXBuffer
    STDMETHOD_(LPVOID, GetBufferPointer)(THIS) PURE;
    STDMETHOD_(DWORD, GetBufferSize)(THIS) PURE;
};



//////////////////////////////////////////////////////////////////////////////
// D3DXSPRITE flags:
// -----------------
// D3DXSPRITE_DONOTSAVESTATE
//   Specifies device state is not to be saved and restored in Begin/End.
// D3DXSPRITE_DONOTMODIFY_RENDERSTATE
//   Specifies device render state is not to be changed in Begin.  The device
//   is assumed to be in a valid state to draw vertices containing POSITION0, 
//   TEXCOORD0, and COLOR0 data.
// D3DXSPRITE_OBJECTSPACE
//   The WORLD, VIEW, and PROJECTION transforms are NOT modified.  The 
//   transforms currently set to the device are used to transform the sprites 
//   when the batch is drawn (at Flush or End).  If this is not specified, 
//   WORLD, VIEW, and PROJECTION transforms are modified so that sprites are 
//   drawn in screenspace coordinates.
// D3DXSPRITE_BILLBOARD
//   Rotates each sprite about its center so that it is facing the viewer.
// D3DXSPRITE_ALPHABLEND
//   Enables ALPHABLEND(SRCALPHA, INVSRCALPHA) and ALPHATEST(alpha > 0).
//   ID3DXFont expects this to be set when drawing text.
// D3DXSPRITE_SORT_TEXTURE
//   Sprites are sorted by texture prior to drawing.  This is recommended when
//   drawing non-overlapping sprites of uniform depth.  For example, drawing
//   screen-aligned text with ID3DXFont.
// D3DXSPRITE_SORT_DEPTH_FRONTTOBACK
//   Sprites are sorted by depth front-to-back prior to drawing.  This is 
//   recommended when drawing opaque sprites of varying depths.
// D3DXSPRITE_SORT_DEPTH_BACKTOFRONT
//   Sprites are sorted by depth back-to-front prior to drawing.  This is 
//   recommended when drawing transparent sprites of varying depths.
// D3DXSPRITE_DO_NOT_ADDREF_TEXTURE
//   Disables calling AddRef() on every draw, and Release() on Flush() for
//   better performance.
//////////////////////////////////////////////////////////////////////////////

#define D3DXSPRITE_DONOTSAVESTATE               (1 << 0)
#define D3DXSPRITE_DONOTMODIFY_RENDERSTATE      (1 << 1)
#define D3DXSPRITE_OBJECTSPACE                  (1 << 2)
#define D3DXSPRITE_BILLBOARD                    (1 << 3)
#define D3DXSPRITE_ALPHABLEND                   (1 << 4)
#define D3DXSPRITE_SORT_TEXTURE                 (1 << 5)
#define D3DXSPRITE_SORT_DEPTH_FRONTTOBACK       (1 << 6)
#define D3DXSPRITE_SORT_DEPTH_BACKTOFRONT       (1 << 7)
#define D3DXSPRITE_DO_NOT_ADDREF_TEXTURE        (1 << 8)


//////////////////////////////////////////////////////////////////////////////
// ID3DXSprite:
// ------------
// This object intends to provide an easy way to drawing sprites using D3D.
//
// Begin - 
//    Prepares device for drawing sprites.
//
// Draw -
//    Draws a sprite.  Before transformation, the sprite is the size of 
//    SrcRect, with its top-left corner specified by Position.  The color 
//    and alpha channels are modulated by Color.
//
// Flush -
//    Forces all batched sprites to submitted to the device.
//
// End - 
//    Restores device state to how it was when Begin was called.
//
// OnLostDevice, OnResetDevice -
//    Call OnLostDevice() on this object before calling Reset() on the
//    device, so that this object can release any stateblocks and video
//    memory resources.  After Reset(), the call OnResetDevice().
//////////////////////////////////////////////////////////////////////////////

typedef interface ID3DXSprite ID3DXSprite;
typedef interface ID3DXSprite *LPD3DXSPRITE;


// {BA0B762D-7D28-43ec-B9DC-2F84443B0614}
DEFINE_GUID(IID_ID3DXSprite, 
0xba0b762d, 0x7d28, 0x43ec, 0xb9, 0xdc, 0x2f, 0x84, 0x44, 0x3b, 0x6, 0x14);


#undef INTERFACE
#define INTERFACE ID3DXSprite

DECLARE_INTERFACE_(ID3DXSprite, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // ID3DXSprite
    STDMETHOD(GetDevice)(THIS_ LPDIRECT3DDEVICE9* ppDevice) PURE;

    STDMETHOD(GetTransform)(THIS_ D3DXMATRIX *pTransform) PURE;
    STDMETHOD(SetTransform)(THIS_ CONST D3DXMATRIX *pTransform) PURE;

    STDMETHOD(SetWorldViewRH)(THIS_ CONST D3DXMATRIX *pWorld, CONST D3DXMATRIX *pView) PURE;
    STDMETHOD(SetWorldViewLH)(THIS_ CONST D3DXMATRIX *pWorld, CONST D3DXMATRIX *pView) PURE;

    STDMETHOD(Begin)(THIS_ DWORD Flags) PURE;
    STDMETHOD(Draw)(THIS_ LPDIRECT3DTEXTURE9 pTexture, CONST RECT *pSrcRect, CONST D3DXVECTOR3 *pCenter, CONST D3DXVECTOR3 *pPosition, D3DCOLOR Color) PURE;
    STDMETHOD(Flush)(THIS) PURE;
    STDMETHOD(End)(THIS) PURE;

    STDMETHOD(OnLostDevice)(THIS) PURE;
    STDMETHOD(OnResetDevice)(THIS) PURE;
};


#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

HRESULT WINAPI 
    D3DXCreateSprite( 
        LPDIRECT3DDEVICE9   pDevice, 
        LPD3DXSPRITE*       ppSprite);

#ifdef __cplusplus
}
#endif //__cplusplus



//////////////////////////////////////////////////////////////////////////////
// ID3DXFont:
// ----------
// Font objects contain the textures and resources needed to render a specific 
// font on a specific device.
//
// GetGlyphData -
//    Returns glyph cache data, for a given glyph.
//
// PreloadCharacters/PreloadGlyphs/PreloadText -
//    Preloads glyphs into the glyph cache textures.
//
// DrawText -
//    Draws formatted text on a D3D device.  Some parameters are 
//    surprisingly similar to those of GDI's DrawText function.  See GDI 
//    documentation for a detailed description of these parameters.
//    If pSprite is NULL, an internal sprite object will be used.
//
// OnLostDevice, OnResetDevice -
//    Call OnLostDevice() on this object before calling Reset() on the
//    device, so that this object can release any stateblocks and video
//    memory resources.  After Reset(), the call OnResetDevice().
//////////////////////////////////////////////////////////////////////////////

typedef struct _D3DXFONT_DESCA
{
    INT Height;
    UINT Width;
    UINT Weight;
    UINT MipLevels;
    BOOL Italic;
    BYTE CharSet;
    BYTE OutputPrecision;
    BYTE Quality;
    BYTE PitchAndFamily;
    CHAR FaceName[LF_FACESIZE];

} D3DXFONT_DESCA, *LPD3DXFONT_DESCA;

typedef struct _D3DXFONT_DESCW
{
    INT Height;
    UINT Width;
    UINT Weight;
    UINT MipLevels;
    BOOL Italic;
    BYTE CharSet;
    BYTE OutputPrecision;
    BYTE Quality;
    BYTE PitchAndFamily;
    WCHAR FaceName[LF_FACESIZE];

} D3DXFONT_DESCW, *LPD3DXFONT_DESCW;

#ifdef UNICODE
typedef D3DXFONT_DESCW D3DXFONT_DESC;
typedef LPD3DXFONT_DESCW LPD3DXFONT_DESC;
#else
typedef D3DXFONT_DESCA D3DXFONT_DESC;
typedef LPD3DXFONT_DESCA LPD3DXFONT_DESC;
#endif


typedef interface ID3DXFont ID3DXFont;
typedef interface ID3DXFont *LPD3DXFONT;


// {D79DBB70-5F21-4d36-BBC2-FF525C213CDC}
DEFINE_GUID(IID_ID3DXFont, 
0xd79dbb70, 0x5f21, 0x4d36, 0xbb, 0xc2, 0xff, 0x52, 0x5c, 0x21, 0x3c, 0xdc);


#undef INTERFACE
#define INTERFACE ID3DXFont

DECLARE_INTERFACE_(ID3DXFont, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // ID3DXFont
    STDMETHOD(GetDevice)(THIS_ LPDIRECT3DDEVICE9 *ppDevice) PURE;
    STDMETHOD(GetDescA)(THIS_ D3DXFONT_DESCA *pDesc) PURE;
    STDMETHOD(GetDescW)(THIS_ D3DXFONT_DESCW *pDesc) PURE;
    STDMETHOD_(BOOL, GetTextMetricsA)(THIS_ TEXTMETRICA *pTextMetrics) PURE;
    STDMETHOD_(BOOL, GetTextMetricsW)(THIS_ TEXTMETRICW *pTextMetrics) PURE;

    STDMETHOD_(HDC, GetDC)(THIS) PURE;
    STDMETHOD(GetGlyphData)(THIS_ UINT Glyph, LPDIRECT3DTEXTURE9 *ppTexture, RECT *pBlackBox, POINT *pCellInc) PURE;

    STDMETHOD(PreloadCharacters)(THIS_ UINT First, UINT Last) PURE;
    STDMETHOD(PreloadGlyphs)(THIS_ UINT First, UINT Last) PURE;
    STDMETHOD(PreloadTextA)(THIS_ LPCSTR pString, INT Count) PURE;
    STDMETHOD(PreloadTextW)(THIS_ LPCWSTR pString, INT Count) PURE;

    STDMETHOD_(INT, DrawTextA)(THIS_ LPD3DXSPRITE pSprite, LPCSTR pString, INT Count, LPRECT pRect, DWORD Format, D3DCOLOR Color) PURE;
    STDMETHOD_(INT, DrawTextW)(THIS_ LPD3DXSPRITE pSprite, LPCWSTR pString, INT Count, LPRECT pRect, DWORD Format, D3DCOLOR Color) PURE;

    STDMETHOD(OnLostDevice)(THIS) PURE;
    STDMETHOD(OnResetDevice)(THIS) PURE;

#ifdef __cplusplus
#ifdef UNICODE
    HRESULT GetDesc(D3DXFONT_DESCW *pDesc) { return GetDescW(pDesc); }
    HRESULT PreloadText(LPCWSTR pString, INT Count) { return PreloadTextW(pString, Count); }
#else
    HRESULT GetDesc(D3DXFONT_DESCA *pDesc) { return GetDescA(pDesc); }
    HRESULT PreloadText(LPCSTR pString, INT Count) { return PreloadTextA(pString, Count); }
#endif
#endif //__cplusplus
};

#ifndef GetTextMetrics
#ifdef UNICODE
#define GetTextMetrics GetTextMetricsW
#else
#define GetTextMetrics GetTextMetricsA
#endif
#endif

#ifndef DrawText
#ifdef UNICODE
#define DrawText DrawTextW
#else
#define DrawText DrawTextA
#endif
#endif


#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


HRESULT WINAPI 
    D3DXCreateFontA(
        LPDIRECT3DDEVICE9       pDevice,  
        INT                     Height,
        UINT                    Width,
        UINT                    Weight,
        UINT                    MipLevels,
        BOOL                    Italic,
        DWORD                   CharSet,
        DWORD                   OutputPrecision,
        DWORD                   Quality,
        DWORD                   PitchAndFamily,
        LPCSTR                  pFaceName,
        LPD3DXFONT*             ppFont);

HRESULT WINAPI 
    D3DXCreateFontW(
        LPDIRECT3DDEVICE9       pDevice,  
        INT                     Height,
        UINT                    Width,
        UINT                    Weight,
        UINT                    MipLevels,
        BOOL                    Italic,
        DWORD                   CharSet,
        DWORD                   OutputPrecision,
        DWORD                   Quality,
        DWORD                   PitchAndFamily,
        LPCWSTR                 pFaceName,
        LPD3DXFONT*             ppFont);

#ifdef UNICODE
#define D3DXCreateFont D3DXCreateFontW
#else
#define D3DXCreateFont D3DXCreateFontA
#endif


HRESULT WINAPI 
    D3DXCreateFontIndirectA( 
        LPDIRECT3DDEVICE9       pDevice, 
        CONST D3DXFONT_DESCA*   pDesc, 
        LPD3DXFONT*             ppFont);

HRESULT WINAPI 
    D3DXCreateFontIndirectW( 
        LPDIRECT3DDEVICE9       pDevice, 
        CONST D3DXFONT_DESCW*   pDesc, 
        LPD3DXFONT*             ppFont);

#ifdef UNICODE
#define D3DXCreateFontIndirect D3DXCreateFontIndirectW
#else
#define D3DXCreateFontIndirect D3DXCreateFontIndirectA
#endif


#ifdef __cplusplus
}
#endif //__cplusplus



///////////////////////////////////////////////////////////////////////////
// ID3DXRenderToSurface:
// ---------------------
// This object abstracts rendering to surfaces.  These surfaces do not 
// necessarily need to be render targets.  If they are not, a compatible
// render target is used, and the result copied into surface at end scene.
//
// BeginScene, EndScene -
//    Call BeginScene() and EndScene() at the beginning and ending of your
//    scene.  These calls will setup and restore render targets, viewports, 
//    etc.. 
//
// OnLostDevice, OnResetDevice -
//    Call OnLostDevice() on this object before calling Reset() on the
//    device, so that this object can release any stateblocks and video
//    memory resources.  After Reset(), the call OnResetDevice().
///////////////////////////////////////////////////////////////////////////

typedef struct _D3DXRTS_DESC
{
    UINT                Width;
    UINT                Height;
    D3DFORMAT           Format;
    BOOL                DepthStencil;
    D3DFORMAT           DepthStencilFormat;

} D3DXRTS_DESC, *LPD3DXRTS_DESC;


typedef interface ID3DXRenderToSurface ID3DXRenderToSurface;
typedef interface ID3DXRenderToSurface *LPD3DXRENDERTOSURFACE;


// {6985F346-2C3D-43b3-BE8B-DAAE8A03D894}
DEFINE_GUID(IID_ID3DXRenderToSurface, 
0x6985f346, 0x2c3d, 0x43b3, 0xbe, 0x8b, 0xda, 0xae, 0x8a, 0x3, 0xd8, 0x94);


#undef INTERFACE
#define INTERFACE ID3DXRenderToSurface

DECLARE_INTERFACE_(ID3DXRenderToSurface, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // ID3DXRenderToSurface
    STDMETHOD(GetDevice)(THIS_ LPDIRECT3DDEVICE9* ppDevice) PURE;
    STDMETHOD(GetDesc)(THIS_ D3DXRTS_DESC* pDesc) PURE;

    STDMETHOD(BeginScene)(THIS_ LPDIRECT3DSURFACE9 pSurface, CONST D3DVIEWPORT9* pViewport) PURE;
    STDMETHOD(EndScene)(THIS_ DWORD MipFilter) PURE;

    STDMETHOD(OnLostDevice)(THIS) PURE;
    STDMETHOD(OnResetDevice)(THIS) PURE;
};


#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

HRESULT WINAPI
    D3DXCreateRenderToSurface(
        LPDIRECT3DDEVICE9       pDevice,
        UINT                    Width,
        UINT                    Height,
        D3DFORMAT               Format,
        BOOL                    DepthStencil,
        D3DFORMAT               DepthStencilFormat,
        LPD3DXRENDERTOSURFACE*  ppRenderToSurface);

#ifdef __cplusplus
}
#endif //__cplusplus



///////////////////////////////////////////////////////////////////////////
// ID3DXRenderToEnvMap:
// --------------------
// This object abstracts rendering to environment maps.  These surfaces 
// do not necessarily need to be render targets.  If they are not, a 
// compatible render target is used, and the result copied into the
// environment map at end scene.
//
// BeginCube, BeginSphere, BeginHemisphere, BeginParabolic -
//    This function initiates the rendering of the environment map.  As
//    parameters, you pass the textures in which will get filled in with
//    the resulting environment map.
//
// Face -
//    Call this function to initiate the drawing of each face.  For each 
//    environment map, you will call this six times.. once for each face 
//    in D3DCUBEMAP_FACES.
//
// End -
//    This will restore all render targets, and if needed compose all the
//    rendered faces into the environment map surfaces.
//
// OnLostDevice, OnResetDevice -
//    Call OnLostDevice() on this object before calling Reset() on the
//    device, so that this object can release any stateblocks and video
//    memory resources.  After Reset(), the call OnResetDevice().
///////////////////////////////////////////////////////////////////////////

typedef struct _D3DXRTE_DESC
{
    UINT        Size;
    UINT        MipLevels;
    D3DFORMAT   Format;
    BOOL        DepthStencil;
    D3DFORMAT   DepthStencilFormat;

} D3DXRTE_DESC, *LPD3DXRTE_DESC;


typedef interface ID3DXRenderToEnvMap ID3DXRenderToEnvMap;
typedef interface ID3DXRenderToEnvMap *LPD3DXRenderToEnvMap;


// {313F1B4B-C7B0-4fa2-9D9D-8D380B64385E}
DEFINE_GUID(IID_ID3DXRenderToEnvMap, 
0x313f1b4b, 0xc7b0, 0x4fa2, 0x9d, 0x9d, 0x8d, 0x38, 0xb, 0x64, 0x38, 0x5e);


#undef INTERFACE
#define INTERFACE ID3DXRenderToEnvMap

DECLARE_INTERFACE_(ID3DXRenderToEnvMap, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // ID3DXRenderToEnvMap
    STDMETHOD(GetDevice)(THIS_ LPDIRECT3DDEVICE9* ppDevice) PURE;
    STDMETHOD(GetDesc)(THIS_ D3DXRTE_DESC* pDesc) PURE;

    STDMETHOD(BeginCube)(THIS_ 
        LPDIRECT3DCUBETEXTURE9 pCubeTex) PURE;

    STDMETHOD(BeginSphere)(THIS_
        LPDIRECT3DTEXTURE9 pTex) PURE;

    STDMETHOD(BeginHemisphere)(THIS_ 
        LPDIRECT3DTEXTURE9 pTexZPos,
        LPDIRECT3DTEXTURE9 pTexZNeg) PURE;

    STDMETHOD(BeginParabolic)(THIS_ 
        LPDIRECT3DTEXTURE9 pTexZPos,
        LPDIRECT3DTEXTURE9 pTexZNeg) PURE;

    STDMETHOD(Face)(THIS_ D3DCUBEMAP_FACES Face, DWORD MipFilter) PURE;
    STDMETHOD(End)(THIS_ DWORD MipFilter) PURE;

    STDMETHOD(OnLostDevice)(THIS) PURE;
    STDMETHOD(OnResetDevice)(THIS) PURE;
};


#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

HRESULT WINAPI
    D3DXCreateRenderToEnvMap(
        LPDIRECT3DDEVICE9       pDevice,
        UINT                    Size,
        UINT                    MipLevels,
        D3DFORMAT               Format,
        BOOL                    DepthStencil,
        D3DFORMAT               DepthStencilFormat,
        LPD3DXRenderToEnvMap*   ppRenderToEnvMap);

#ifdef __cplusplus
}
#endif //__cplusplus



///////////////////////////////////////////////////////////////////////////
// ID3DXLine:
// ------------
// This object intends to provide an easy way to draw lines using D3D.
//
// Begin - 
//    Prepares device for drawing lines
//
// Draw -
//    Draws a line strip in screen-space.
//    Input is in the form of a array defining points on the line strip. of D3DXVECTOR2 
//
// DrawTransform -
//    Draws a line in screen-space with a specified input transformation matrix.
//
// End - 
//     Restores device state to how it was when Begin was called.
//
// SetPattern - 
//     Applies a stipple pattern to the line.  Input is one 32-bit
//     DWORD which describes the stipple pattern. 1 is opaque, 0 is
//     transparent.
//
// SetPatternScale - 
//     Stretches the stipple pattern in the u direction.  Input is one
//     floating-point value.  0.0f is no scaling, whereas 1.0f doubles
//     the length of the stipple pattern.
//
// SetWidth - 
//     Specifies the thickness of the line in the v direction.  Input is
//     one floating-point value.
//
// SetAntialias - 
//     Toggles line antialiasing.  Input is a BOOL.
//     TRUE  = Antialiasing on.
//     FALSE = Antialiasing off.
//
// SetGLLines - 
//     Toggles non-antialiased OpenGL line emulation.  Input is a BOOL.
//     TRUE  = OpenGL line emulation on.
//     FALSE = OpenGL line emulation off.
//
// OpenGL line:     Regular line:  
//   *\                *\
//   | \              /  \
//   |  \            *\   \
//   *\  \             \   \
//     \  \             \   \
//      \  *             \   *
//       \ |              \ /
//        \|               *
//         *
//
// OnLostDevice, OnResetDevice -
//    Call OnLostDevice() on this object before calling Reset() on the
//    device, so that this object can release any stateblocks and video
//    memory resources.  After Reset(), the call OnResetDevice().
///////////////////////////////////////////////////////////////////////////


typedef interface ID3DXLine ID3DXLine;
typedef interface ID3DXLine *LPD3DXLINE;


// {D379BA7F-9042-4ac4-9F5E-58192A4C6BD8}
DEFINE_GUID(IID_ID3DXLine, 
0xd379ba7f, 0x9042, 0x4ac4, 0x9f, 0x5e, 0x58, 0x19, 0x2a, 0x4c, 0x6b, 0xd8);

#undef INTERFACE
#define INTERFACE ID3DXLine

DECLARE_INTERFACE_(ID3DXLine, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // ID3DXLine
    STDMETHOD(GetDevice)(THIS_ LPDIRECT3DDEVICE9* ppDevice) PURE;

    STDMETHOD(Begin)(THIS) PURE;

    STDMETHOD(Draw)(THIS_ CONST D3DXVECTOR2 *pVertexList,
        DWORD dwVertexListCount, D3DCOLOR Color) PURE;

    STDMETHOD(DrawTransform)(THIS_ CONST D3DXVECTOR3 *pVertexList,
        DWORD dwVertexListCount, CONST D3DXMATRIX* pTransform, 
        D3DCOLOR Color) PURE;

    STDMETHOD(SetPattern)(THIS_ DWORD dwPattern) PURE;
    STDMETHOD_(DWORD, GetPattern)(THIS) PURE;

    STDMETHOD(SetPatternScale)(THIS_ FLOAT fPatternScale) PURE;
    STDMETHOD_(FLOAT, GetPatternScale)(THIS) PURE;

    STDMETHOD(SetWidth)(THIS_ FLOAT fWidth) PURE;
    STDMETHOD_(FLOAT, GetWidth)(THIS) PURE;

    STDMETHOD(SetAntialias)(THIS_ BOOL bAntialias) PURE;
    STDMETHOD_(BOOL, GetAntialias)(THIS) PURE;

    STDMETHOD(SetGLLines)(THIS_ BOOL bGLLines) PURE;
    STDMETHOD_(BOOL, GetGLLines)(THIS) PURE;

    STDMETHOD(End)(THIS) PURE;

    STDMETHOD(OnLostDevice)(THIS) PURE;
    STDMETHOD(OnResetDevice)(THIS) PURE;
};


#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


HRESULT WINAPI
    D3DXCreateLine(
        LPDIRECT3DDEVICE9   pDevice,
        LPD3DXLINE*         ppLine);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //__D3DX9CORE_H__


//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) Microsoft Corporation.  All Rights Reserved.
//
//  File:       d3dx9math.h
//  Content:    D3DX math types and functions
//
//////////////////////////////////////////////////////////////////////////////

#ifndef __D3DX9MATH_H__
#define __D3DX9MATH_H__

#include <math.h>
#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4201) // anonymous unions warning



//===========================================================================
//
// General purpose utilities
//
//===========================================================================
#define D3DX_PI    ((FLOAT)  3.141592654f)
#define D3DX_1BYPI ((FLOAT)  0.318309886f)

#define D3DXToRadian( degree ) ((degree) * (D3DX_PI / 180.0f))
#define D3DXToDegree( radian ) ((radian) * (180.0f / D3DX_PI))



//===========================================================================
//
// 16 bit floating point numbers
//
//===========================================================================

#define D3DX_16F_DIG          3                // # of decimal digits of precision
#define D3DX_16F_EPSILON      4.8875809e-4f    // smallest such that 1.0 + epsilon != 1.0
#define D3DX_16F_MANT_DIG     11               // # of bits in mantissa
#define D3DX_16F_MAX          6.550400e+004    // max value
#define D3DX_16F_MAX_10_EXP   4                // max decimal exponent
#define D3DX_16F_MAX_EXP      15               // max binary exponent
#define D3DX_16F_MIN          6.1035156e-5f    // min positive value
#define D3DX_16F_MIN_10_EXP   (-4)             // min decimal exponent
#define D3DX_16F_MIN_EXP      (-14)            // min binary exponent
#define D3DX_16F_RADIX        2                // exponent radix
#define D3DX_16F_ROUNDS       1                // addition rounding: near


typedef struct D3DXFLOAT16
{
#ifdef __cplusplus
public:
    D3DXFLOAT16() {};
    D3DXFLOAT16( FLOAT );
    D3DXFLOAT16( CONST D3DXFLOAT16& );

    // casting
    operator FLOAT ();

    // binary operators
    BOOL operator == ( CONST D3DXFLOAT16& ) const;
    BOOL operator != ( CONST D3DXFLOAT16& ) const;

protected:
#endif //__cplusplus
    WORD value;
} D3DXFLOAT16, *LPD3DXFLOAT16;



//===========================================================================
//
// Vectors
//
//===========================================================================


//--------------------------
// 2D Vector
//--------------------------
typedef struct D3DXVECTOR2
{
#ifdef __cplusplus
public:
    D3DXVECTOR2() {};
    D3DXVECTOR2( CONST FLOAT * );
    D3DXVECTOR2( CONST D3DXFLOAT16 * );
    D3DXVECTOR2( FLOAT x, FLOAT y );

    // casting
    operator FLOAT* ();
    operator CONST FLOAT* () const;

    // assignment operators
    D3DXVECTOR2& operator += ( CONST D3DXVECTOR2& );
    D3DXVECTOR2& operator -= ( CONST D3DXVECTOR2& );
    D3DXVECTOR2& operator *= ( FLOAT );
    D3DXVECTOR2& operator /= ( FLOAT );

    // unary operators
    D3DXVECTOR2 operator + () const;
    D3DXVECTOR2 operator - () const;

    // binary operators
    D3DXVECTOR2 operator + ( CONST D3DXVECTOR2& ) const;
    D3DXVECTOR2 operator - ( CONST D3DXVECTOR2& ) const;
    D3DXVECTOR2 operator * ( FLOAT ) const;
    D3DXVECTOR2 operator / ( FLOAT ) const;

    friend D3DXVECTOR2 operator * ( FLOAT, CONST D3DXVECTOR2& );

    BOOL operator == ( CONST D3DXVECTOR2& ) const;
    BOOL operator != ( CONST D3DXVECTOR2& ) const;


public:
#endif //__cplusplus
    FLOAT x, y;
} D3DXVECTOR2, *LPD3DXVECTOR2;



//--------------------------
// 2D Vector (16 bit)
//--------------------------

typedef struct D3DXVECTOR2_16F
{
#ifdef __cplusplus
public:
    D3DXVECTOR2_16F() {};
    D3DXVECTOR2_16F( CONST FLOAT * );
    D3DXVECTOR2_16F( CONST D3DXFLOAT16 * );
    D3DXVECTOR2_16F( CONST D3DXFLOAT16 &x, CONST D3DXFLOAT16 &y );

    // casting
    operator D3DXFLOAT16* ();
    operator CONST D3DXFLOAT16* () const;

    // binary operators
    BOOL operator == ( CONST D3DXVECTOR2_16F& ) const;
    BOOL operator != ( CONST D3DXVECTOR2_16F& ) const;

public:
#endif //__cplusplus
    D3DXFLOAT16 x, y;

} D3DXVECTOR2_16F, *LPD3DXVECTOR2_16F;



//--------------------------
// 3D Vector
//--------------------------
#ifdef __cplusplus
typedef struct D3DXVECTOR3 : public D3DVECTOR
{
public:
    D3DXVECTOR3() {};
    D3DXVECTOR3( CONST FLOAT * );
    D3DXVECTOR3( CONST D3DVECTOR& );
    D3DXVECTOR3( CONST D3DXFLOAT16 * );
    D3DXVECTOR3( FLOAT x, FLOAT y, FLOAT z );

    // casting
    operator FLOAT* ();
    operator CONST FLOAT* () const;

    // assignment operators
    D3DXVECTOR3& operator += ( CONST D3DXVECTOR3& );
    D3DXVECTOR3& operator -= ( CONST D3DXVECTOR3& );
    D3DXVECTOR3& operator *= ( FLOAT );
    D3DXVECTOR3& operator /= ( FLOAT );

    // unary operators
    D3DXVECTOR3 operator + () const;
    D3DXVECTOR3 operator - () const;

    // binary operators
    D3DXVECTOR3 operator + ( CONST D3DXVECTOR3& ) const;
    D3DXVECTOR3 operator - ( CONST D3DXVECTOR3& ) const;
    D3DXVECTOR3 operator * ( FLOAT ) const;
    D3DXVECTOR3 operator / ( FLOAT ) const;

    friend D3DXVECTOR3 operator * ( FLOAT, CONST struct D3DXVECTOR3& );

    BOOL operator == ( CONST D3DXVECTOR3& ) const;
    BOOL operator != ( CONST D3DXVECTOR3& ) const;

} D3DXVECTOR3, *LPD3DXVECTOR3;

#else //!__cplusplus
typedef struct _D3DVECTOR D3DXVECTOR3, *LPD3DXVECTOR3;
#endif //!__cplusplus



//--------------------------
// 3D Vector (16 bit)
//--------------------------
typedef struct D3DXVECTOR3_16F
{
#ifdef __cplusplus
public:
    D3DXVECTOR3_16F() {};
    D3DXVECTOR3_16F( CONST FLOAT * );
    D3DXVECTOR3_16F( CONST D3DVECTOR& );
    D3DXVECTOR3_16F( CONST D3DXFLOAT16 * );
    D3DXVECTOR3_16F( CONST D3DXFLOAT16 &x, CONST D3DXFLOAT16 &y, CONST D3DXFLOAT16 &z );

    // casting
    operator D3DXFLOAT16* ();
    operator CONST D3DXFLOAT16* () const;

    // binary operators
    BOOL operator == ( CONST D3DXVECTOR3_16F& ) const;
    BOOL operator != ( CONST D3DXVECTOR3_16F& ) const;

public:
#endif //__cplusplus
    D3DXFLOAT16 x, y, z;

} D3DXVECTOR3_16F, *LPD3DXVECTOR3_16F;



//--------------------------
// 4D Vector
//--------------------------
typedef struct D3DXVECTOR4
{
#ifdef __cplusplus
public:
    D3DXVECTOR4() {};
    D3DXVECTOR4( CONST FLOAT* );
    D3DXVECTOR4( CONST D3DXFLOAT16* );
    D3DXVECTOR4( CONST D3DVECTOR& xyz, FLOAT w );
    D3DXVECTOR4( FLOAT x, FLOAT y, FLOAT z, FLOAT w );

    // casting
    operator FLOAT* ();
    operator CONST FLOAT* () const;

    // assignment operators
    D3DXVECTOR4& operator += ( CONST D3DXVECTOR4& );
    D3DXVECTOR4& operator -= ( CONST D3DXVECTOR4& );
    D3DXVECTOR4& operator *= ( FLOAT );
    D3DXVECTOR4& operator /= ( FLOAT );

    // unary operators
    D3DXVECTOR4 operator + () const;
    D3DXVECTOR4 operator - () const;

    // binary operators
    D3DXVECTOR4 operator + ( CONST D3DXVECTOR4& ) const;
    D3DXVECTOR4 operator - ( CONST D3DXVECTOR4& ) const;
    D3DXVECTOR4 operator * ( FLOAT ) const;
    D3DXVECTOR4 operator / ( FLOAT ) const;

    friend D3DXVECTOR4 operator * ( FLOAT, CONST D3DXVECTOR4& );

    BOOL operator == ( CONST D3DXVECTOR4& ) const;
    BOOL operator != ( CONST D3DXVECTOR4& ) const;

public:
#endif //__cplusplus
    FLOAT x, y, z, w;
} D3DXVECTOR4, *LPD3DXVECTOR4;


//--------------------------
// 4D Vector (16 bit)
//--------------------------
typedef struct D3DXVECTOR4_16F
{
#ifdef __cplusplus
public:
    D3DXVECTOR4_16F() {};
    D3DXVECTOR4_16F( CONST FLOAT * );
    D3DXVECTOR4_16F( CONST D3DXFLOAT16* );
    D3DXVECTOR4_16F( CONST D3DXVECTOR3_16F& xyz, CONST D3DXFLOAT16& w );
    D3DXVECTOR4_16F( CONST D3DXFLOAT16& x, CONST D3DXFLOAT16& y, CONST D3DXFLOAT16& z, CONST D3DXFLOAT16& w );

    // casting
    operator D3DXFLOAT16* ();
    operator CONST D3DXFLOAT16* () const;

    // binary operators
    BOOL operator == ( CONST D3DXVECTOR4_16F& ) const;
    BOOL operator != ( CONST D3DXVECTOR4_16F& ) const;

public:
#endif //__cplusplus
    D3DXFLOAT16 x, y, z, w;

} D3DXVECTOR4_16F, *LPD3DXVECTOR4_16F;



//===========================================================================
//
// Matrices
//
//===========================================================================
#ifdef __cplusplus
typedef struct D3DXMATRIX : public D3DMATRIX
{
public:
    D3DXMATRIX() {};
    D3DXMATRIX( CONST FLOAT * );
    D3DXMATRIX( CONST D3DMATRIX& );
    D3DXMATRIX( CONST D3DXFLOAT16 * );
    D3DXMATRIX( FLOAT _11, FLOAT _12, FLOAT _13, FLOAT _14,
                FLOAT _21, FLOAT _22, FLOAT _23, FLOAT _24,
                FLOAT _31, FLOAT _32, FLOAT _33, FLOAT _34,
                FLOAT _41, FLOAT _42, FLOAT _43, FLOAT _44 );


    // access grants
    FLOAT& operator () ( UINT Row, UINT Col );
    FLOAT  operator () ( UINT Row, UINT Col ) const;

    // casting operators
    operator FLOAT* ();
    operator CONST FLOAT* () const;

    // assignment operators
    D3DXMATRIX& operator *= ( CONST D3DXMATRIX& );
    D3DXMATRIX& operator += ( CONST D3DXMATRIX& );
    D3DXMATRIX& operator -= ( CONST D3DXMATRIX& );
    D3DXMATRIX& operator *= ( FLOAT );
    D3DXMATRIX& operator /= ( FLOAT );

    // unary operators
    D3DXMATRIX operator + () const;
    D3DXMATRIX operator - () const;

    // binary operators
    D3DXMATRIX operator * ( CONST D3DXMATRIX& ) const;
    D3DXMATRIX operator + ( CONST D3DXMATRIX& ) const;
    D3DXMATRIX operator - ( CONST D3DXMATRIX& ) const;
    D3DXMATRIX operator * ( FLOAT ) const;
    D3DXMATRIX operator / ( FLOAT ) const;

    friend D3DXMATRIX operator * ( FLOAT, CONST D3DXMATRIX& );

    BOOL operator == ( CONST D3DXMATRIX& ) const;
    BOOL operator != ( CONST D3DXMATRIX& ) const;

} D3DXMATRIX, *LPD3DXMATRIX;

#else //!__cplusplus
typedef struct _D3DMATRIX D3DXMATRIX, *LPD3DXMATRIX;
#endif //!__cplusplus


//---------------------------------------------------------------------------
// Aligned Matrices
//
// This class helps keep matrices 16-byte aligned as preferred by P4 cpus.
// It aligns matrices on the stack and on the heap or in global scope.
// It does this using __declspec(align(16)) which works on VC7 and on VC 6
// with the processor pack. Unfortunately there is no way to detect the 
// latter so this is turned on only on VC7. On other compilers this is the
// the same as D3DXMATRIX.
//
// Using this class on a compiler that does not actually do the alignment
// can be dangerous since it will not expose bugs that ignore alignment.
// E.g if an object of this class in inside a struct or class, and some code
// memcopys data in it assuming tight packing. This could break on a compiler
// that eventually start aligning the matrix.
//---------------------------------------------------------------------------
#ifdef __cplusplus
typedef struct _D3DXMATRIXA16 : public D3DXMATRIX
{
    _D3DXMATRIXA16() {}
    _D3DXMATRIXA16( CONST FLOAT * );
    _D3DXMATRIXA16( CONST D3DMATRIX& );
    _D3DXMATRIXA16( CONST D3DXFLOAT16 * );
    _D3DXMATRIXA16( FLOAT _11, FLOAT _12, FLOAT _13, FLOAT _14,
                    FLOAT _21, FLOAT _22, FLOAT _23, FLOAT _24,
                    FLOAT _31, FLOAT _32, FLOAT _33, FLOAT _34,
                    FLOAT _41, FLOAT _42, FLOAT _43, FLOAT _44 );

    // new operators
    void* operator new   ( size_t );
    void* operator new[] ( size_t );

    // delete operators
    void operator delete   ( void* );   // These are NOT virtual; Do not 
    void operator delete[] ( void* );   // cast to D3DXMATRIX and delete.
    
    // assignment operators
    _D3DXMATRIXA16& operator = ( CONST D3DXMATRIX& );

} _D3DXMATRIXA16;

#else //!__cplusplus
typedef D3DXMATRIX  _D3DXMATRIXA16;
#endif //!__cplusplus



#if _MSC_VER >= 1300  // VC7
#define D3DX_ALIGN16 __declspec(align(16))
#else
#define D3DX_ALIGN16  // Earlier compiler may not understand this, do nothing.
#endif

typedef D3DX_ALIGN16 _D3DXMATRIXA16 D3DXMATRIXA16, *LPD3DXMATRIXA16;



//===========================================================================
//
//    Quaternions
//
//===========================================================================
typedef struct D3DXQUATERNION
{
#ifdef __cplusplus
public:
    D3DXQUATERNION() {}
    D3DXQUATERNION( CONST FLOAT * );
    D3DXQUATERNION( CONST D3DXFLOAT16 * );
    D3DXQUATERNION( FLOAT x, FLOAT y, FLOAT z, FLOAT w );

    // casting
    operator FLOAT* ();
    operator CONST FLOAT* () const;

    // assignment operators
    D3DXQUATERNION& operator += ( CONST D3DXQUATERNION& );
    D3DXQUATERNION& operator -= ( CONST D3DXQUATERNION& );
    D3DXQUATERNION& operator *= ( CONST D3DXQUATERNION& );
    D3DXQUATERNION& operator *= ( FLOAT );
    D3DXQUATERNION& operator /= ( FLOAT );

    // unary operators
    D3DXQUATERNION  operator + () const;
    D3DXQUATERNION  operator - () const;

    // binary operators
    D3DXQUATERNION operator + ( CONST D3DXQUATERNION& ) const;
    D3DXQUATERNION operator - ( CONST D3DXQUATERNION& ) const;
    D3DXQUATERNION operator * ( CONST D3DXQUATERNION& ) const;
    D3DXQUATERNION operator * ( FLOAT ) const;
    D3DXQUATERNION operator / ( FLOAT ) const;

    friend D3DXQUATERNION operator * (FLOAT, CONST D3DXQUATERNION& );

    BOOL operator == ( CONST D3DXQUATERNION& ) const;
    BOOL operator != ( CONST D3DXQUATERNION& ) const;

#endif //__cplusplus
    FLOAT x, y, z, w;
} D3DXQUATERNION, *LPD3DXQUATERNION;


//===========================================================================
//
// Planes
//
//===========================================================================
typedef struct D3DXPLANE
{
#ifdef __cplusplus
public:
    D3DXPLANE() {}
    D3DXPLANE( CONST FLOAT* );
    D3DXPLANE( CONST D3DXFLOAT16* );
    D3DXPLANE( FLOAT a, FLOAT b, FLOAT c, FLOAT d );

    // casting
    operator FLOAT* ();
    operator CONST FLOAT* () const;

    // assignment operators
    D3DXPLANE& operator *= ( FLOAT );
    D3DXPLANE& operator /= ( FLOAT );

    // unary operators
    D3DXPLANE operator + () const;
    D3DXPLANE operator - () const;

    // binary operators
    D3DXPLANE operator * ( FLOAT ) const;
    D3DXPLANE operator / ( FLOAT ) const;

    friend D3DXPLANE operator * ( FLOAT, CONST D3DXPLANE& );

    BOOL operator == ( CONST D3DXPLANE& ) const;
    BOOL operator != ( CONST D3DXPLANE& ) const;

#endif //__cplusplus
    FLOAT a, b, c, d;
} D3DXPLANE, *LPD3DXPLANE;


//===========================================================================
//
// Colors
//
//===========================================================================

typedef struct D3DXCOLOR
{
#ifdef __cplusplus
public:
    D3DXCOLOR() {}
    D3DXCOLOR( DWORD argb );
    D3DXCOLOR( CONST FLOAT * );
    D3DXCOLOR( CONST D3DXFLOAT16 * );
    D3DXCOLOR( CONST D3DCOLORVALUE& );
    D3DXCOLOR( FLOAT r, FLOAT g, FLOAT b, FLOAT a );

    // casting
    operator DWORD () const;

    operator FLOAT* ();
    operator CONST FLOAT* () const;

    operator D3DCOLORVALUE* ();
    operator CONST D3DCOLORVALUE* () const;

    operator D3DCOLORVALUE& ();
    operator CONST D3DCOLORVALUE& () const;

    // assignment operators
    D3DXCOLOR& operator += ( CONST D3DXCOLOR& );
    D3DXCOLOR& operator -= ( CONST D3DXCOLOR& );
    D3DXCOLOR& operator *= ( FLOAT );
    D3DXCOLOR& operator /= ( FLOAT );

    // unary operators
    D3DXCOLOR operator + () const;
    D3DXCOLOR operator - () const;

    // binary operators
    D3DXCOLOR operator + ( CONST D3DXCOLOR& ) const;
    D3DXCOLOR operator - ( CONST D3DXCOLOR& ) const;
    D3DXCOLOR operator * ( FLOAT ) const;
    D3DXCOLOR operator / ( FLOAT ) const;

    friend D3DXCOLOR operator * ( FLOAT, CONST D3DXCOLOR& );

    BOOL operator == ( CONST D3DXCOLOR& ) const;
    BOOL operator != ( CONST D3DXCOLOR& ) const;

#endif //__cplusplus
    FLOAT r, g, b, a;
} D3DXCOLOR, *LPD3DXCOLOR;



//===========================================================================
//
// D3DX math functions:
//
// NOTE:
//  * All these functions can take the same object as in and out parameters.
//
//  * Out parameters are typically also returned as return values, so that
//    the output of one function may be used as a parameter to another.
//
//===========================================================================

//--------------------------
// Float16
//--------------------------

// non-inline
#ifdef __cplusplus
extern "C" {
#endif

// Converts an array 32-bit floats to 16-bit floats
D3DXFLOAT16* WINAPI D3DXFloat32To16Array
    ( D3DXFLOAT16 *pOut, CONST FLOAT *pIn, UINT n );

// Converts an array 16-bit floats to 32-bit floats
FLOAT* WINAPI D3DXFloat16To32Array
    ( FLOAT *pOut, CONST D3DXFLOAT16 *pIn, UINT n );

#ifdef __cplusplus
}
#endif


//--------------------------
// 2D Vector
//--------------------------

// inline

FLOAT D3DXVec2Length
    ( CONST D3DXVECTOR2 *pV );

FLOAT D3DXVec2LengthSq
    ( CONST D3DXVECTOR2 *pV );

FLOAT D3DXVec2Dot
    ( CONST D3DXVECTOR2 *pV1, CONST D3DXVECTOR2 *pV2 );

// Z component of ((x1,y1,0) cross (x2,y2,0))
FLOAT D3DXVec2CCW
    ( CONST D3DXVECTOR2 *pV1, CONST D3DXVECTOR2 *pV2 );

D3DXVECTOR2* D3DXVec2Add
    ( D3DXVECTOR2 *pOut, CONST D3DXVECTOR2 *pV1, CONST D3DXVECTOR2 *pV2 );

D3DXVECTOR2* D3DXVec2Subtract
    ( D3DXVECTOR2 *pOut, CONST D3DXVECTOR2 *pV1, CONST D3DXVECTOR2 *pV2 );

// Minimize each component.  x = min(x1, x2), y = min(y1, y2)
D3DXVECTOR2* D3DXVec2Minimize
    ( D3DXVECTOR2 *pOut, CONST D3DXVECTOR2 *pV1, CONST D3DXVECTOR2 *pV2 );

// Maximize each component.  x = max(x1, x2), y = max(y1, y2)
D3DXVECTOR2* D3DXVec2Maximize
    ( D3DXVECTOR2 *pOut, CONST D3DXVECTOR2 *pV1, CONST D3DXVECTOR2 *pV2 );

D3DXVECTOR2* D3DXVec2Scale
    ( D3DXVECTOR2 *pOut, CONST D3DXVECTOR2 *pV, FLOAT s );

// Linear interpolation. V1 + s(V2-V1)
D3DXVECTOR2* D3DXVec2Lerp
    ( D3DXVECTOR2 *pOut, CONST D3DXVECTOR2 *pV1, CONST D3DXVECTOR2 *pV2,
      FLOAT s );

// non-inline
#ifdef __cplusplus
extern "C" {
#endif

D3DXVECTOR2* WINAPI D3DXVec2Normalize
    ( D3DXVECTOR2 *pOut, CONST D3DXVECTOR2 *pV );

// Hermite interpolation between position V1, tangent T1 (when s == 0)
// and position V2, tangent T2 (when s == 1).
D3DXVECTOR2* WINAPI D3DXVec2Hermite
    ( D3DXVECTOR2 *pOut, CONST D3DXVECTOR2 *pV1, CONST D3DXVECTOR2 *pT1,
      CONST D3DXVECTOR2 *pV2, CONST D3DXVECTOR2 *pT2, FLOAT s );

// CatmullRom interpolation between V1 (when s == 0) and V2 (when s == 1)
D3DXVECTOR2* WINAPI D3DXVec2CatmullRom
    ( D3DXVECTOR2 *pOut, CONST D3DXVECTOR2 *pV0, CONST D3DXVECTOR2 *pV1,
      CONST D3DXVECTOR2 *pV2, CONST D3DXVECTOR2 *pV3, FLOAT s );

// Barycentric coordinates.  V1 + f(V2-V1) + g(V3-V1)
D3DXVECTOR2* WINAPI D3DXVec2BaryCentric
    ( D3DXVECTOR2 *pOut, CONST D3DXVECTOR2 *pV1, CONST D3DXVECTOR2 *pV2,
      CONST D3DXVECTOR2 *pV3, FLOAT f, FLOAT g);

// Transform (x, y, 0, 1) by matrix.
D3DXVECTOR4* WINAPI D3DXVec2Transform
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR2 *pV, CONST D3DXMATRIX *pM );

// Transform (x, y, 0, 1) by matrix, project result back into w=1.
D3DXVECTOR2* WINAPI D3DXVec2TransformCoord
    ( D3DXVECTOR2 *pOut, CONST D3DXVECTOR2 *pV, CONST D3DXMATRIX *pM );

// Transform (x, y, 0, 0) by matrix.
D3DXVECTOR2* WINAPI D3DXVec2TransformNormal
    ( D3DXVECTOR2 *pOut, CONST D3DXVECTOR2 *pV, CONST D3DXMATRIX *pM );
     
// Transform Array (x, y, 0, 1) by matrix.
D3DXVECTOR4* WINAPI D3DXVec2TransformArray
    ( D3DXVECTOR4 *pOut, UINT OutStride, CONST D3DXVECTOR2 *pV, UINT VStride, CONST D3DXMATRIX *pM, UINT n);

// Transform Array (x, y, 0, 1) by matrix, project result back into w=1.
D3DXVECTOR2* WINAPI D3DXVec2TransformCoordArray
    ( D3DXVECTOR2 *pOut, UINT OutStride, CONST D3DXVECTOR2 *pV, UINT VStride, CONST D3DXMATRIX *pM, UINT n );

// Transform Array (x, y, 0, 0) by matrix.
D3DXVECTOR2* WINAPI D3DXVec2TransformNormalArray
    ( D3DXVECTOR2 *pOut, UINT OutStride, CONST D3DXVECTOR2 *pV, UINT VStride, CONST D3DXMATRIX *pM, UINT n );
    
    

#ifdef __cplusplus
}
#endif


//--------------------------
// 3D Vector
//--------------------------

// inline

FLOAT D3DXVec3Length
    ( CONST D3DXVECTOR3 *pV );

FLOAT D3DXVec3LengthSq
    ( CONST D3DXVECTOR3 *pV );

FLOAT D3DXVec3Dot
    ( CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2 );

D3DXVECTOR3* D3DXVec3Cross
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2 );

D3DXVECTOR3* D3DXVec3Add
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2 );

D3DXVECTOR3* D3DXVec3Subtract
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2 );

// Minimize each component.  x = min(x1, x2), y = min(y1, y2), ...
D3DXVECTOR3* D3DXVec3Minimize
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2 );

// Maximize each component.  x = max(x1, x2), y = max(y1, y2), ...
D3DXVECTOR3* D3DXVec3Maximize
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2 );

D3DXVECTOR3* D3DXVec3Scale
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV, FLOAT s);

// Linear interpolation. V1 + s(V2-V1)
D3DXVECTOR3* D3DXVec3Lerp
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2,
      FLOAT s );

// non-inline
#ifdef __cplusplus
extern "C" {
#endif

D3DXVECTOR3* WINAPI D3DXVec3Normalize
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV );

// Hermite interpolation between position V1, tangent T1 (when s == 0)
// and position V2, tangent T2 (when s == 1).
D3DXVECTOR3* WINAPI D3DXVec3Hermite
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pT1,
      CONST D3DXVECTOR3 *pV2, CONST D3DXVECTOR3 *pT2, FLOAT s );

// CatmullRom interpolation between V1 (when s == 0) and V2 (when s == 1)
D3DXVECTOR3* WINAPI D3DXVec3CatmullRom
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV0, CONST D3DXVECTOR3 *pV1,
      CONST D3DXVECTOR3 *pV2, CONST D3DXVECTOR3 *pV3, FLOAT s );

// Barycentric coordinates.  V1 + f(V2-V1) + g(V3-V1)
D3DXVECTOR3* WINAPI D3DXVec3BaryCentric
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2,
      CONST D3DXVECTOR3 *pV3, FLOAT f, FLOAT g);

// Transform (x, y, z, 1) by matrix.
D3DXVECTOR4* WINAPI D3DXVec3Transform
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR3 *pV, CONST D3DXMATRIX *pM );

// Transform (x, y, z, 1) by matrix, project result back into w=1.
D3DXVECTOR3* WINAPI D3DXVec3TransformCoord
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV, CONST D3DXMATRIX *pM );

// Transform (x, y, z, 0) by matrix.  If you transforming a normal by a 
// non-affine matrix, the matrix you pass to this function should be the 
// transpose of the inverse of the matrix you would use to transform a coord.
D3DXVECTOR3* WINAPI D3DXVec3TransformNormal
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV, CONST D3DXMATRIX *pM );
    
    
// Transform Array (x, y, z, 1) by matrix. 
D3DXVECTOR4* WINAPI D3DXVec3TransformArray
    ( D3DXVECTOR4 *pOut, UINT OutStride, CONST D3DXVECTOR3 *pV, UINT VStride, CONST D3DXMATRIX *pM, UINT n );

// Transform Array (x, y, z, 1) by matrix, project result back into w=1.
D3DXVECTOR3* WINAPI D3DXVec3TransformCoordArray
    ( D3DXVECTOR3 *pOut, UINT OutStride, CONST D3DXVECTOR3 *pV, UINT VStride, CONST D3DXMATRIX *pM, UINT n );

// Transform (x, y, z, 0) by matrix.  If you transforming a normal by a 
// non-affine matrix, the matrix you pass to this function should be the 
// transpose of the inverse of the matrix you would use to transform a coord.
D3DXVECTOR3* WINAPI D3DXVec3TransformNormalArray
    ( D3DXVECTOR3 *pOut, UINT OutStride, CONST D3DXVECTOR3 *pV, UINT VStride, CONST D3DXMATRIX *pM, UINT n );

// Project vector from object space into screen space
D3DXVECTOR3* WINAPI D3DXVec3Project
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV, CONST D3DVIEWPORT9 *pViewport,
      CONST D3DXMATRIX *pProjection, CONST D3DXMATRIX *pView, CONST D3DXMATRIX *pWorld);

// Project vector from screen space into object space
D3DXVECTOR3* WINAPI D3DXVec3Unproject
    ( D3DXVECTOR3 *pOut, CONST D3DXVECTOR3 *pV, CONST D3DVIEWPORT9 *pViewport,
      CONST D3DXMATRIX *pProjection, CONST D3DXMATRIX *pView, CONST D3DXMATRIX *pWorld);
      
// Project vector Array from object space into screen space
D3DXVECTOR3* WINAPI D3DXVec3ProjectArray
    ( D3DXVECTOR3 *pOut, UINT OutStride,CONST D3DXVECTOR3 *pV, UINT VStride,CONST D3DVIEWPORT9 *pViewport,
      CONST D3DXMATRIX *pProjection, CONST D3DXMATRIX *pView, CONST D3DXMATRIX *pWorld, UINT n);

// Project vector Array from screen space into object space
D3DXVECTOR3* WINAPI D3DXVec3UnprojectArray
    ( D3DXVECTOR3 *pOut, UINT OutStride, CONST D3DXVECTOR3 *pV, UINT VStride, CONST D3DVIEWPORT9 *pViewport,
      CONST D3DXMATRIX *pProjection, CONST D3DXMATRIX *pView, CONST D3DXMATRIX *pWorld, UINT n);


#ifdef __cplusplus
}
#endif



//--------------------------
// 4D Vector
//--------------------------

// inline

FLOAT D3DXVec4Length
    ( CONST D3DXVECTOR4 *pV );

FLOAT D3DXVec4LengthSq
    ( CONST D3DXVECTOR4 *pV );

FLOAT D3DXVec4Dot
    ( CONST D3DXVECTOR4 *pV1, CONST D3DXVECTOR4 *pV2 );

D3DXVECTOR4* D3DXVec4Add
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV1, CONST D3DXVECTOR4 *pV2);

D3DXVECTOR4* D3DXVec4Subtract
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV1, CONST D3DXVECTOR4 *pV2);

// Minimize each component.  x = min(x1, x2), y = min(y1, y2), ...
D3DXVECTOR4* D3DXVec4Minimize
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV1, CONST D3DXVECTOR4 *pV2);

// Maximize each component.  x = max(x1, x2), y = max(y1, y2), ...
D3DXVECTOR4* D3DXVec4Maximize
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV1, CONST D3DXVECTOR4 *pV2);

D3DXVECTOR4* D3DXVec4Scale
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV, FLOAT s);

// Linear interpolation. V1 + s(V2-V1)
D3DXVECTOR4* D3DXVec4Lerp
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV1, CONST D3DXVECTOR4 *pV2,
      FLOAT s );

// non-inline
#ifdef __cplusplus
extern "C" {
#endif

// Cross-product in 4 dimensions.
D3DXVECTOR4* WINAPI D3DXVec4Cross
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV1, CONST D3DXVECTOR4 *pV2,
      CONST D3DXVECTOR4 *pV3);

D3DXVECTOR4* WINAPI D3DXVec4Normalize
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV );

// Hermite interpolation between position V1, tangent T1 (when s == 0)
// and position V2, tangent T2 (when s == 1).
D3DXVECTOR4* WINAPI D3DXVec4Hermite
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV1, CONST D3DXVECTOR4 *pT1,
      CONST D3DXVECTOR4 *pV2, CONST D3DXVECTOR4 *pT2, FLOAT s );

// CatmullRom interpolation between V1 (when s == 0) and V2 (when s == 1)
D3DXVECTOR4* WINAPI D3DXVec4CatmullRom
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV0, CONST D3DXVECTOR4 *pV1,
      CONST D3DXVECTOR4 *pV2, CONST D3DXVECTOR4 *pV3, FLOAT s );

// Barycentric coordinates.  V1 + f(V2-V1) + g(V3-V1)
D3DXVECTOR4* WINAPI D3DXVec4BaryCentric
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV1, CONST D3DXVECTOR4 *pV2,
      CONST D3DXVECTOR4 *pV3, FLOAT f, FLOAT g);

// Transform vector by matrix.
D3DXVECTOR4* WINAPI D3DXVec4Transform
    ( D3DXVECTOR4 *pOut, CONST D3DXVECTOR4 *pV, CONST D3DXMATRIX *pM );
    
// Transform vector array by matrix.
D3DXVECTOR4* WINAPI D3DXVec4TransformArray
    ( D3DXVECTOR4 *pOut, UINT OutStride, CONST D3DXVECTOR4 *pV, UINT VStride, CONST D3DXMATRIX *pM, UINT n );

#ifdef __cplusplus
}
#endif


//--------------------------
// 4D Matrix
//--------------------------

// inline

D3DXMATRIX* D3DXMatrixIdentity
    ( D3DXMATRIX *pOut );

BOOL D3DXMatrixIsIdentity
    ( CONST D3DXMATRIX *pM );


// non-inline
#ifdef __cplusplus
extern "C" {
#endif

FLOAT WINAPI D3DXMatrixDeterminant
    ( CONST D3DXMATRIX *pM );

HRESULT WINAPI D3DXMatrixDecompose
    ( D3DXVECTOR3 *pOutScale, D3DXQUATERNION *pOutRotation, 
	  D3DXVECTOR3 *pOutTranslation, CONST D3DXMATRIX *pM );

D3DXMATRIX* WINAPI D3DXMatrixTranspose
    ( D3DXMATRIX *pOut, CONST D3DXMATRIX *pM );

// Matrix multiplication.  The result represents the transformation M2
// followed by the transformation M1.  (Out = M1 * M2)
D3DXMATRIX* WINAPI D3DXMatrixMultiply
    ( D3DXMATRIX *pOut, CONST D3DXMATRIX *pM1, CONST D3DXMATRIX *pM2 );

// Matrix multiplication, followed by a transpose. (Out = T(M1 * M2))
D3DXMATRIX* WINAPI D3DXMatrixMultiplyTranspose
    ( D3DXMATRIX *pOut, CONST D3DXMATRIX *pM1, CONST D3DXMATRIX *pM2 );

// Calculate inverse of matrix.  Inversion my fail, in which case NULL will
// be returned.  The determinant of pM is also returned it pfDeterminant
// is non-NULL.
D3DXMATRIX* WINAPI D3DXMatrixInverse
    ( D3DXMATRIX *pOut, FLOAT *pDeterminant, CONST D3DXMATRIX *pM );

// Build a matrix which scales by (sx, sy, sz)
D3DXMATRIX* WINAPI D3DXMatrixScaling
    ( D3DXMATRIX *pOut, FLOAT sx, FLOAT sy, FLOAT sz );

// Build a matrix which translates by (x, y, z)
D3DXMATRIX* WINAPI D3DXMatrixTranslation
    ( D3DXMATRIX *pOut, FLOAT x, FLOAT y, FLOAT z );

// Build a matrix which rotates around the X axis
D3DXMATRIX* WINAPI D3DXMatrixRotationX
    ( D3DXMATRIX *pOut, FLOAT Angle );

// Build a matrix which rotates around the Y axis
D3DXMATRIX* WINAPI D3DXMatrixRotationY
    ( D3DXMATRIX *pOut, FLOAT Angle );

// Build a matrix which rotates around the Z axis
D3DXMATRIX* WINAPI D3DXMatrixRotationZ
    ( D3DXMATRIX *pOut, FLOAT Angle );

// Build a matrix which rotates around an arbitrary axis
D3DXMATRIX* WINAPI D3DXMatrixRotationAxis
    ( D3DXMATRIX *pOut, CONST D3DXVECTOR3 *pV, FLOAT Angle );

// Build a matrix from a quaternion
D3DXMATRIX* WINAPI D3DXMatrixRotationQuaternion
    ( D3DXMATRIX *pOut, CONST D3DXQUATERNION *pQ);

// Yaw around the Y axis, a pitch around the X axis,
// and a roll around the Z axis.
D3DXMATRIX* WINAPI D3DXMatrixRotationYawPitchRoll
    ( D3DXMATRIX *pOut, FLOAT Yaw, FLOAT Pitch, FLOAT Roll );

// Build transformation matrix.  NULL arguments are treated as identity.
// Mout = Msc-1 * Msr-1 * Ms * Msr * Msc * Mrc-1 * Mr * Mrc * Mt
D3DXMATRIX* WINAPI D3DXMatrixTransformation
    ( D3DXMATRIX *pOut, CONST D3DXVECTOR3 *pScalingCenter,
      CONST D3DXQUATERNION *pScalingRotation, CONST D3DXVECTOR3 *pScaling,
      CONST D3DXVECTOR3 *pRotationCenter, CONST D3DXQUATERNION *pRotation,
      CONST D3DXVECTOR3 *pTranslation);

// Build 2D transformation matrix in XY plane.  NULL arguments are treated as identity.
// Mout = Msc-1 * Msr-1 * Ms * Msr * Msc * Mrc-1 * Mr * Mrc * Mt
D3DXMATRIX* WINAPI D3DXMatrixTransformation2D
    ( D3DXMATRIX *pOut, CONST D3DXVECTOR2* pScalingCenter, 
      FLOAT ScalingRotation, CONST D3DXVECTOR2* pScaling, 
      CONST D3DXVECTOR2* pRotationCenter, FLOAT Rotation, 
      CONST D3DXVECTOR2* pTranslation);

// Build affine transformation matrix.  NULL arguments are treated as identity.
// Mout = Ms * Mrc-1 * Mr * Mrc * Mt
D3DXMATRIX* WINAPI D3DXMatrixAffineTransformation
    ( D3DXMATRIX *pOut, FLOAT Scaling, CONST D3DXVECTOR3 *pRotationCenter,
      CONST D3DXQUATERNION *pRotation, CONST D3DXVECTOR3 *pTranslation);

// Build 2D affine transformation matrix in XY plane.  NULL arguments are treated as identity.
// Mout = Ms * Mrc-1 * Mr * Mrc * Mt
D3DXMATRIX* WINAPI D3DXMatrixAffineTransformation2D
    ( D3DXMATRIX *pOut, FLOAT Scaling, CONST D3DXVECTOR2* pRotationCenter, 
      FLOAT Rotation, CONST D3DXVECTOR2* pTranslation);

// Build a lookat matrix. (right-handed)
D3DXMATRIX* WINAPI D3DXMatrixLookAtRH
    ( D3DXMATRIX *pOut, CONST D3DXVECTOR3 *pEye, CONST D3DXVECTOR3 *pAt,
      CONST D3DXVECTOR3 *pUp );

// Build a lookat matrix. (left-handed)
D3DXMATRIX* WINAPI D3DXMatrixLookAtLH
    ( D3DXMATRIX *pOut, CONST D3DXVECTOR3 *pEye, CONST D3DXVECTOR3 *pAt,
      CONST D3DXVECTOR3 *pUp );

// Build a perspective projection matrix. (right-handed)
D3DXMATRIX* WINAPI D3DXMatrixPerspectiveRH
    ( D3DXMATRIX *pOut, FLOAT w, FLOAT h, FLOAT zn, FLOAT zf );

// Build a perspective projection matrix. (left-handed)
D3DXMATRIX* WINAPI D3DXMatrixPerspectiveLH
    ( D3DXMATRIX *pOut, FLOAT w, FLOAT h, FLOAT zn, FLOAT zf );

// Build a perspective projection matrix. (right-handed)
D3DXMATRIX* WINAPI D3DXMatrixPerspectiveFovRH
    ( D3DXMATRIX *pOut, FLOAT fovy, FLOAT Aspect, FLOAT zn, FLOAT zf );

// Build a perspective projection matrix. (left-handed)
D3DXMATRIX* WINAPI D3DXMatrixPerspectiveFovLH
    ( D3DXMATRIX *pOut, FLOAT fovy, FLOAT Aspect, FLOAT zn, FLOAT zf );

// Build a perspective projection matrix. (right-handed)
D3DXMATRIX* WINAPI D3DXMatrixPerspectiveOffCenterRH
    ( D3DXMATRIX *pOut, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn,
      FLOAT zf );

// Build a perspective projection matrix. (left-handed)
D3DXMATRIX* WINAPI D3DXMatrixPerspectiveOffCenterLH
    ( D3DXMATRIX *pOut, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn,
      FLOAT zf );

// Build an ortho projection matrix. (right-handed)
D3DXMATRIX* WINAPI D3DXMatrixOrthoRH
    ( D3DXMATRIX *pOut, FLOAT w, FLOAT h, FLOAT zn, FLOAT zf );

// Build an ortho projection matrix. (left-handed)
D3DXMATRIX* WINAPI D3DXMatrixOrthoLH
    ( D3DXMATRIX *pOut, FLOAT w, FLOAT h, FLOAT zn, FLOAT zf );

// Build an ortho projection matrix. (right-handed)
D3DXMATRIX* WINAPI D3DXMatrixOrthoOffCenterRH
    ( D3DXMATRIX *pOut, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn,
      FLOAT zf );

// Build an ortho projection matrix. (left-handed)
D3DXMATRIX* WINAPI D3DXMatrixOrthoOffCenterLH
    ( D3DXMATRIX *pOut, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn,
      FLOAT zf );

// Build a matrix which flattens geometry into a plane, as if casting
// a shadow from a light.
D3DXMATRIX* WINAPI D3DXMatrixShadow
    ( D3DXMATRIX *pOut, CONST D3DXVECTOR4 *pLight,
      CONST D3DXPLANE *pPlane );

// Build a matrix which reflects the coordinate system about a plane
D3DXMATRIX* WINAPI D3DXMatrixReflect
    ( D3DXMATRIX *pOut, CONST D3DXPLANE *pPlane );

#ifdef __cplusplus
}
#endif


//--------------------------
// Quaternion
//--------------------------

// inline

FLOAT D3DXQuaternionLength
    ( CONST D3DXQUATERNION *pQ );

// Length squared, or "norm"
FLOAT D3DXQuaternionLengthSq
    ( CONST D3DXQUATERNION *pQ );

FLOAT D3DXQuaternionDot
    ( CONST D3DXQUATERNION *pQ1, CONST D3DXQUATERNION *pQ2 );

// (0, 0, 0, 1)
D3DXQUATERNION* D3DXQuaternionIdentity
    ( D3DXQUATERNION *pOut );

BOOL D3DXQuaternionIsIdentity
    ( CONST D3DXQUATERNION *pQ );

// (-x, -y, -z, w)
D3DXQUATERNION* D3DXQuaternionConjugate
    ( D3DXQUATERNION *pOut, CONST D3DXQUATERNION *pQ );


// non-inline
#ifdef __cplusplus
extern "C" {
#endif

// Compute a quaternin's axis and angle of rotation. Expects unit quaternions.
void WINAPI D3DXQuaternionToAxisAngle
    ( CONST D3DXQUATERNION *pQ, D3DXVECTOR3 *pAxis, FLOAT *pAngle );

// Build a quaternion from a rotation matrix.
D3DXQUATERNION* WINAPI D3DXQuaternionRotationMatrix
    ( D3DXQUATERNION *pOut, CONST D3DXMATRIX *pM);

// Rotation about arbitrary axis.
D3DXQUATERNION* WINAPI D3DXQuaternionRotationAxis
    ( D3DXQUATERNION *pOut, CONST D3DXVECTOR3 *pV, FLOAT Angle );

// Yaw around the Y axis, a pitch around the X axis,
// and a roll around the Z axis.
D3DXQUATERNION* WINAPI D3DXQuaternionRotationYawPitchRoll
    ( D3DXQUATERNION *pOut, FLOAT Yaw, FLOAT Pitch, FLOAT Roll );

// Quaternion multiplication.  The result represents the rotation Q2
// followed by the rotation Q1.  (Out = Q2 * Q1)
D3DXQUATERNION* WINAPI D3DXQuaternionMultiply
    ( D3DXQUATERNION *pOut, CONST D3DXQUATERNION *pQ1,
      CONST D3DXQUATERNION *pQ2 );

D3DXQUATERNION* WINAPI D3DXQuaternionNormalize
    ( D3DXQUATERNION *pOut, CONST D3DXQUATERNION *pQ );

// Conjugate and re-norm
D3DXQUATERNION* WINAPI D3DXQuaternionInverse
    ( D3DXQUATERNION *pOut, CONST D3DXQUATERNION *pQ );

// Expects unit quaternions.
// if q = (cos(theta), sin(theta) * v); ln(q) = (0, theta * v)
D3DXQUATERNION* WINAPI D3DXQuaternionLn
    ( D3DXQUATERNION *pOut, CONST D3DXQUATERNION *pQ );

// Expects pure quaternions. (w == 0)  w is ignored in calculation.
// if q = (0, theta * v); exp(q) = (cos(theta), sin(theta) * v)
D3DXQUATERNION* WINAPI D3DXQuaternionExp
    ( D3DXQUATERNION *pOut, CONST D3DXQUATERNION *pQ );
      
// Spherical linear interpolation between Q1 (t == 0) and Q2 (t == 1).
// Expects unit quaternions.
D3DXQUATERNION* WINAPI D3DXQuaternionSlerp
    ( D3DXQUATERNION *pOut, CONST D3DXQUATERNION *pQ1,
      CONST D3DXQUATERNION *pQ2, FLOAT t );

// Spherical quadrangle interpolation.
// Slerp(Slerp(Q1, C, t), Slerp(A, B, t), 2t(1-t))
D3DXQUATERNION* WINAPI D3DXQuaternionSquad
    ( D3DXQUATERNION *pOut, CONST D3DXQUATERNION *pQ1,
      CONST D3DXQUATERNION *pA, CONST D3DXQUATERNION *pB,
      CONST D3DXQUATERNION *pC, FLOAT t );

// Setup control points for spherical quadrangle interpolation
// from Q1 to Q2.  The control points are chosen in such a way 
// to ensure the continuity of tangents with adjacent segments.
void WINAPI D3DXQuaternionSquadSetup
    ( D3DXQUATERNION *pAOut, D3DXQUATERNION *pBOut, D3DXQUATERNION *pCOut,
      CONST D3DXQUATERNION *pQ0, CONST D3DXQUATERNION *pQ1, 
      CONST D3DXQUATERNION *pQ2, CONST D3DXQUATERNION *pQ3 );

// Barycentric interpolation.
// Slerp(Slerp(Q1, Q2, f+g), Slerp(Q1, Q3, f+g), g/(f+g))
D3DXQUATERNION* WINAPI D3DXQuaternionBaryCentric
    ( D3DXQUATERNION *pOut, CONST D3DXQUATERNION *pQ1,
      CONST D3DXQUATERNION *pQ2, CONST D3DXQUATERNION *pQ3,
      FLOAT f, FLOAT g );

#ifdef __cplusplus
}
#endif


//--------------------------
// Plane
//--------------------------

// inline

// ax + by + cz + dw
FLOAT D3DXPlaneDot
    ( CONST D3DXPLANE *pP, CONST D3DXVECTOR4 *pV);

// ax + by + cz + d
FLOAT D3DXPlaneDotCoord
    ( CONST D3DXPLANE *pP, CONST D3DXVECTOR3 *pV);

// ax + by + cz
FLOAT D3DXPlaneDotNormal
    ( CONST D3DXPLANE *pP, CONST D3DXVECTOR3 *pV);

D3DXPLANE* D3DXPlaneScale
    (D3DXPLANE *pOut, CONST D3DXPLANE *pP, FLOAT s);

// non-inline
#ifdef __cplusplus
extern "C" {
#endif

// Normalize plane (so that |a,b,c| == 1)
D3DXPLANE* WINAPI D3DXPlaneNormalize
    ( D3DXPLANE *pOut, CONST D3DXPLANE *pP);

// Find the intersection between a plane and a line.  If the line is
// parallel to the plane, NULL is returned.
D3DXVECTOR3* WINAPI D3DXPlaneIntersectLine
    ( D3DXVECTOR3 *pOut, CONST D3DXPLANE *pP, CONST D3DXVECTOR3 *pV1,
      CONST D3DXVECTOR3 *pV2);

// Construct a plane from a point and a normal
D3DXPLANE* WINAPI D3DXPlaneFromPointNormal
    ( D3DXPLANE *pOut, CONST D3DXVECTOR3 *pPoint, CONST D3DXVECTOR3 *pNormal);

// Construct a plane from 3 points
D3DXPLANE* WINAPI D3DXPlaneFromPoints
    ( D3DXPLANE *pOut, CONST D3DXVECTOR3 *pV1, CONST D3DXVECTOR3 *pV2,
      CONST D3DXVECTOR3 *pV3);

// Transform a plane by a matrix.  The vector (a,b,c) must be normal.
// M should be the inverse transpose of the transformation desired.
D3DXPLANE* WINAPI D3DXPlaneTransform
    ( D3DXPLANE *pOut, CONST D3DXPLANE *pP, CONST D3DXMATRIX *pM );
    
// Transform an array of planes by a matrix.  The vectors (a,b,c) must be normal.
// M should be the inverse transpose of the transformation desired.
D3DXPLANE* WINAPI D3DXPlaneTransformArray
    ( D3DXPLANE *pOut, UINT OutStride, CONST D3DXPLANE *pP, UINT PStride, CONST D3DXMATRIX *pM, UINT n );

#ifdef __cplusplus
}
#endif


//--------------------------
// Color
//--------------------------

// inline

// (1-r, 1-g, 1-b, a)
D3DXCOLOR* D3DXColorNegative
    (D3DXCOLOR *pOut, CONST D3DXCOLOR *pC);

D3DXCOLOR* D3DXColorAdd
    (D3DXCOLOR *pOut, CONST D3DXCOLOR *pC1, CONST D3DXCOLOR *pC2);

D3DXCOLOR* D3DXColorSubtract
    (D3DXCOLOR *pOut, CONST D3DXCOLOR *pC1, CONST D3DXCOLOR *pC2);

D3DXCOLOR* D3DXColorScale
    (D3DXCOLOR *pOut, CONST D3DXCOLOR *pC, FLOAT s);

// (r1*r2, g1*g2, b1*b2, a1*a2)
D3DXCOLOR* D3DXColorModulate
    (D3DXCOLOR *pOut, CONST D3DXCOLOR *pC1, CONST D3DXCOLOR *pC2);

// Linear interpolation of r,g,b, and a. C1 + s(C2-C1)
D3DXCOLOR* D3DXColorLerp
    (D3DXCOLOR *pOut, CONST D3DXCOLOR *pC1, CONST D3DXCOLOR *pC2, FLOAT s);

// non-inline
#ifdef __cplusplus
extern "C" {
#endif

// Interpolate r,g,b between desaturated color and color.
// DesaturatedColor + s(Color - DesaturatedColor)
D3DXCOLOR* WINAPI D3DXColorAdjustSaturation
    (D3DXCOLOR *pOut, CONST D3DXCOLOR *pC, FLOAT s);

// Interpolate r,g,b between 50% grey and color.  Grey + s(Color - Grey)
D3DXCOLOR* WINAPI D3DXColorAdjustContrast
    (D3DXCOLOR *pOut, CONST D3DXCOLOR *pC, FLOAT c);

#ifdef __cplusplus
}
#endif




//--------------------------
// Misc
//--------------------------

#ifdef __cplusplus
extern "C" {
#endif

// Calculate Fresnel term given the cosine of theta (likely obtained by
// taking the dot of two normals), and the refraction index of the material.
FLOAT WINAPI D3DXFresnelTerm
    (FLOAT CosTheta, FLOAT RefractionIndex);     

#ifdef __cplusplus
}
#endif



//===========================================================================
//
//    Matrix Stack
//
//===========================================================================

typedef interface ID3DXMatrixStack ID3DXMatrixStack;
typedef interface ID3DXMatrixStack *LPD3DXMATRIXSTACK;

// {C7885BA7-F990-4fe7-922D-8515E477DD85}
DEFINE_GUID(IID_ID3DXMatrixStack, 
0xc7885ba7, 0xf990, 0x4fe7, 0x92, 0x2d, 0x85, 0x15, 0xe4, 0x77, 0xdd, 0x85);


#undef INTERFACE
#define INTERFACE ID3DXMatrixStack

DECLARE_INTERFACE_(ID3DXMatrixStack, IUnknown)
{
    //
    // IUnknown methods
    //
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    //
    // ID3DXMatrixStack methods
    //

    // Pops the top of the stack, returns the current top
    // *after* popping the top.
    STDMETHOD(Pop)(THIS) PURE;

    // Pushes the stack by one, duplicating the current matrix.
    STDMETHOD(Push)(THIS) PURE;

    // Loads identity in the current matrix.
    STDMETHOD(LoadIdentity)(THIS) PURE;

    // Loads the given matrix into the current matrix
    STDMETHOD(LoadMatrix)(THIS_ CONST D3DXMATRIX* pM ) PURE;

    // Right-Multiplies the given matrix to the current matrix.
    // (transformation is about the current world origin)
    STDMETHOD(MultMatrix)(THIS_ CONST D3DXMATRIX* pM ) PURE;

    // Left-Multiplies the given matrix to the current matrix
    // (transformation is about the local origin of the object)
    STDMETHOD(MultMatrixLocal)(THIS_ CONST D3DXMATRIX* pM ) PURE;

    // Right multiply the current matrix with the computed rotation
    // matrix, counterclockwise about the given axis with the given angle.
    // (rotation is about the current world origin)
    STDMETHOD(RotateAxis)
        (THIS_ CONST D3DXVECTOR3* pV, FLOAT Angle) PURE;

    // Left multiply the current matrix with the computed rotation
    // matrix, counterclockwise about the given axis with the given angle.
    // (rotation is about the local origin of the object)
    STDMETHOD(RotateAxisLocal)
        (THIS_ CONST D3DXVECTOR3* pV, FLOAT Angle) PURE;

    // Right multiply the current matrix with the computed rotation
    // matrix. All angles are counterclockwise. (rotation is about the
    // current world origin)

    // The rotation is composed of a yaw around the Y axis, a pitch around
    // the X axis, and a roll around the Z axis.
    STDMETHOD(RotateYawPitchRoll)
        (THIS_ FLOAT Yaw, FLOAT Pitch, FLOAT Roll) PURE;

    // Left multiply the current matrix with the computed rotation
    // matrix. All angles are counterclockwise. (rotation is about the
    // local origin of the object)

    // The rotation is composed of a yaw around the Y axis, a pitch around
    // the X axis, and a roll around the Z axis.
    STDMETHOD(RotateYawPitchRollLocal)
        (THIS_ FLOAT Yaw, FLOAT Pitch, FLOAT Roll) PURE;

    // Right multiply the current matrix with the computed scale
    // matrix. (transformation is about the current world origin)
    STDMETHOD(Scale)(THIS_ FLOAT x, FLOAT y, FLOAT z) PURE;

    // Left multiply the current matrix with the computed scale
    // matrix. (transformation is about the local origin of the object)
    STDMETHOD(ScaleLocal)(THIS_ FLOAT x, FLOAT y, FLOAT z) PURE;

    // Right multiply the current matrix with the computed translation
    // matrix. (transformation is about the current world origin)
    STDMETHOD(Translate)(THIS_ FLOAT x, FLOAT y, FLOAT z ) PURE;

    // Left multiply the current matrix with the computed translation
    // matrix. (transformation is about the local origin of the object)
    STDMETHOD(TranslateLocal)(THIS_ FLOAT x, FLOAT y, FLOAT z) PURE;

    // Obtain the current matrix at the top of the stack
    STDMETHOD_(D3DXMATRIX*, GetTop)(THIS) PURE;
};

#ifdef __cplusplus
extern "C" {
#endif

HRESULT WINAPI 
    D3DXCreateMatrixStack( 
        DWORD               Flags, 
        LPD3DXMATRIXSTACK*  ppStack);

#ifdef __cplusplus
}
#endif

//===========================================================================
//
//  Spherical Harmonic Runtime Routines
//
// NOTE:
//  * Most of these functions can take the same object as in and out parameters.
//    The exceptions are the rotation functions.  
//
//  * Out parameters are typically also returned as return values, so that
//    the output of one function may be used as a parameter to another.
//
//============================================================================


// non-inline
#ifdef __cplusplus
extern "C" {
#endif

//============================================================================
//
//  Basic Spherical Harmonic math routines
//
//============================================================================

#define D3DXSH_MINORDER 2
#define D3DXSH_MAXORDER 6

//============================================================================
//
//  D3DXSHEvalDirection:
//  --------------------
//  Evaluates the Spherical Harmonic basis functions
//
//  Parameters:
//   pOut
//      Output SH coefficients - basis function Ylm is stored at l*l + m+l
//      This is the pointer that is returned.
//   Order
//      Order of the SH evaluation, generates Order^2 coefs, degree is Order-1
//   pDir
//      Direction to evaluate in - assumed to be normalized
//
//============================================================================

FLOAT* WINAPI D3DXSHEvalDirection
    (  FLOAT *pOut, UINT Order, CONST D3DXVECTOR3 *pDir );
    
//============================================================================
//
//  D3DXSHRotate:
//  --------------------
//  Rotates SH vector by a rotation matrix
//
//  Parameters:
//   pOut
//      Output SH coefficients - basis function Ylm is stored at l*l + m+l
//      This is the pointer that is returned (should not alias with pIn.)
//   Order
//      Order of the SH evaluation, generates Order^2 coefs, degree is Order-1
//   pMatrix
//      Matrix used for rotation - rotation sub matrix should be orthogonal
//      and have a unit determinant.
//   pIn
//      Input SH coeffs (rotated), incorect results if this is also output.
//
//============================================================================

FLOAT* WINAPI D3DXSHRotate
    ( FLOAT *pOut, UINT Order, CONST D3DXMATRIX *pMatrix, CONST FLOAT *pIn );
    
//============================================================================
//
//  D3DXSHRotateZ:
//  --------------------
//  Rotates the SH vector in the Z axis by an angle
//
//  Parameters:
//   pOut
//      Output SH coefficients - basis function Ylm is stored at l*l + m+l
//      This is the pointer that is returned (should not alias with pIn.)
//   Order
//      Order of the SH evaluation, generates Order^2 coefs, degree is Order-1
//   Angle
//      Angle in radians to rotate around the Z axis.
//   pIn
//      Input SH coeffs (rotated), incorect results if this is also output.
//
//============================================================================


FLOAT* WINAPI D3DXSHRotateZ
    ( FLOAT *pOut, UINT Order, FLOAT Angle, CONST FLOAT *pIn );
    
//============================================================================
//
//  D3DXSHAdd:
//  --------------------
//  Adds two SH vectors, pOut[i] = pA[i] + pB[i];
//
//  Parameters:
//   pOut
//      Output SH coefficients - basis function Ylm is stored at l*l + m+l
//      This is the pointer that is returned.
//   Order
//      Order of the SH evaluation, generates Order^2 coefs, degree is Order-1
//   pA
//      Input SH coeffs.
//   pB
//      Input SH coeffs (second vector.)
//
//============================================================================

FLOAT* WINAPI D3DXSHAdd
    ( FLOAT *pOut, UINT Order, CONST FLOAT *pA, CONST FLOAT *pB );

//============================================================================
//
//  D3DXSHScale:
//  --------------------
//  Adds two SH vectors, pOut[i] = pA[i]*Scale;
//
//  Parameters:
//   pOut
//      Output SH coefficients - basis function Ylm is stored at l*l + m+l
//      This is the pointer that is returned.
//   Order
//      Order of the SH evaluation, generates Order^2 coefs, degree is Order-1
//   pIn
//      Input SH coeffs.
//   Scale
//      Scale factor.
//
//============================================================================

FLOAT* WINAPI D3DXSHScale
    ( FLOAT *pOut, UINT Order, CONST FLOAT *pIn, CONST FLOAT Scale );
    
//============================================================================
//
//  D3DXSHDot:
//  --------------------
//  Computes the dot product of two SH vectors
//
//  Parameters:
//   Order
//      Order of the SH evaluation, generates Order^2 coefs, degree is Order-1
//   pA
//      Input SH coeffs.
//   pB
//      Second set of input SH coeffs.
//
//============================================================================

FLOAT WINAPI D3DXSHDot
    ( UINT Order, CONST FLOAT *pA, CONST FLOAT *pB );

//============================================================================
//
//  D3DXSHMultiply[O]:
//  --------------------
//  Computes the product of two functions represented using SH (f and g), where:
//  pOut[i] = int(y_i(s) * f(s) * g(s)), where y_i(s) is the ith SH basis
//  function, f(s) and g(s) are SH functions (sum_i(y_i(s)*c_i)).  The order O
//  determines the lengths of the arrays, where there should always be O^2 
//  coefficients.  In general the product of two SH functions of order O generates
//  and SH function of order 2*O - 1, but we truncate the result.  This means
//  that the product commutes (f*g == g*f) but doesn't associate 
//  (f*(g*h) != (f*g)*h.
//
//  Parameters:
//   pOut
//      Output SH coefficients - basis function Ylm is stored at l*l + m+l
//      This is the pointer that is returned.
//   pF
//      Input SH coeffs for first function.
//   pG
//      Second set of input SH coeffs.
//
//============================================================================

FLOAT* WINAPI D3DXSHMultiply2( FLOAT *pOut, CONST FLOAT *pF, CONST FLOAT *pG);
FLOAT* WINAPI D3DXSHMultiply3( FLOAT *pOut, CONST FLOAT *pF, CONST FLOAT *pG);
FLOAT* WINAPI D3DXSHMultiply4( FLOAT *pOut, CONST FLOAT *pF, CONST FLOAT *pG);
FLOAT* WINAPI D3DXSHMultiply5( FLOAT *pOut, CONST FLOAT *pF, CONST FLOAT *pG);
FLOAT* WINAPI D3DXSHMultiply6( FLOAT *pOut, CONST FLOAT *pF, CONST FLOAT *pG);


//============================================================================
//
//  Basic Spherical Harmonic lighting routines
//
//============================================================================

//============================================================================
//
//  D3DXSHEvalDirectionalLight:
//  --------------------
//  Evaluates a directional light and returns spectral SH data.  The output 
//  vector is computed so that if the intensity of R/G/B is unit the resulting
//  exit radiance of a point directly under the light on a diffuse object with
//  an albedo of 1 would be 1.0.  This will compute 3 spectral samples, pROut
//  has to be specified, while pGout and pBout are optional.
//
//  Parameters:
//   Order
//      Order of the SH evaluation, generates Order^2 coefs, degree is Order-1
//   pDir
//      Direction light is coming from (assumed to be normalized.)
//   RIntensity
//      Red intensity of light.
//   GIntensity
//      Green intensity of light.
//   BIntensity
//      Blue intensity of light.
//   pROut
//      Output SH vector for Red.
//   pGOut
//      Output SH vector for Green (optional.)
//   pBOut
//      Output SH vector for Blue (optional.)        
//
//============================================================================

HRESULT WINAPI D3DXSHEvalDirectionalLight
    ( UINT Order, CONST D3DXVECTOR3 *pDir, 
      FLOAT RIntensity, FLOAT GIntensity, FLOAT BIntensity,
      FLOAT *pROut, FLOAT *pGOut, FLOAT *pBOut );

//============================================================================
//
//  D3DXSHEvalSphericalLight:
//  --------------------
//  Evaluates a spherical light and returns spectral SH data.  There is no 
//  normalization of the intensity of the light like there is for directional
//  lights, care has to be taken when specifiying the intensities.  This will 
//  compute 3 spectral samples, pROut has to be specified, while pGout and 
//  pBout are optional.
//
//  Parameters:
//   Order
//      Order of the SH evaluation, generates Order^2 coefs, degree is Order-1
//   pPos
//      Position of light - reciever is assumed to be at the origin.
//   Radius
//      Radius of the spherical light source.
//   RIntensity
//      Red intensity of light.
//   GIntensity
//      Green intensity of light.
//   BIntensity
//      Blue intensity of light.
//   pROut
//      Output SH vector for Red.
//   pGOut
//      Output SH vector for Green (optional.)
//   pBOut
//      Output SH vector for Blue (optional.)        
//
//============================================================================

HRESULT WINAPI D3DXSHEvalSphericalLight
    ( UINT Order, CONST D3DXVECTOR3 *pPos, FLOAT Radius,
      FLOAT RIntensity, FLOAT GIntensity, FLOAT BIntensity,
      FLOAT *pROut, FLOAT *pGOut, FLOAT *pBOut );

//============================================================================
//
//  D3DXSHEvalConeLight:
//  --------------------
//  Evaluates a light that is a cone of constant intensity and returns spectral
//  SH data.  The output vector is computed so that if the intensity of R/G/B is
//  unit the resulting exit radiance of a point directly under the light oriented
//  in the cone direction on a diffuse object with an albedo of 1 would be 1.0.
//  This will compute 3 spectral samples, pROut has to be specified, while pGout
//  and pBout are optional.
//
//  Parameters:
//   Order
//      Order of the SH evaluation, generates Order^2 coefs, degree is Order-1
//   pDir
//      Direction light is coming from (assumed to be normalized.)
//   Radius
//      Radius of cone in radians.
//   RIntensity
//      Red intensity of light.
//   GIntensity
//      Green intensity of light.
//   BIntensity
//      Blue intensity of light.
//   pROut
//      Output SH vector for Red.
//   pGOut
//      Output SH vector for Green (optional.)
//   pBOut
//      Output SH vector for Blue (optional.)        
//
//============================================================================

HRESULT WINAPI D3DXSHEvalConeLight
    ( UINT Order, CONST D3DXVECTOR3 *pDir, FLOAT Radius,
      FLOAT RIntensity, FLOAT GIntensity, FLOAT BIntensity,
      FLOAT *pROut, FLOAT *pGOut, FLOAT *pBOut );
      
//============================================================================
//
//  D3DXSHEvalHemisphereLight:
//  --------------------
//  Evaluates a light that is a linear interpolant between two colors over the
//  sphere.  The interpolant is linear along the axis of the two points, not
//  over the surface of the sphere (ie: if the axis was (0,0,1) it is linear in
//  Z, not in the azimuthal angle.)  The resulting spherical lighting function
//  is normalized so that a point on a perfectly diffuse surface with no
//  shadowing and a normal pointed in the direction pDir would result in exit
//  radiance with a value of 1 if the top color was white and the bottom color
//  was black.  This is a very simple model where Top represents the intensity 
//  of the "sky" and Bottom represents the intensity of the "ground".
//
//  Parameters:
//   Order
//      Order of the SH evaluation, generates Order^2 coefs, degree is Order-1
//   pDir
//      Axis of the hemisphere.
//   Top
//      Color of the upper hemisphere.
//   Bottom
//      Color of the lower hemisphere.
//   pROut
//      Output SH vector for Red.
//   pGOut
//      Output SH vector for Green
//   pBOut
//      Output SH vector for Blue        
//
//============================================================================

HRESULT WINAPI D3DXSHEvalHemisphereLight
    ( UINT Order, CONST D3DXVECTOR3 *pDir, D3DXCOLOR Top, D3DXCOLOR Bottom,
      FLOAT *pROut, FLOAT *pGOut, FLOAT *pBOut );

//============================================================================
//
//  Basic Spherical Harmonic projection routines
//
//============================================================================

//============================================================================
//
//  D3DXSHProjectCubeMap:
//  --------------------
//  Projects a function represented on a cube map into spherical harmonics.
//
//  Parameters:
//   Order
//      Order of the SH evaluation, generates Order^2 coefs, degree is Order-1
//   pCubeMap
//      CubeMap that is going to be projected into spherical harmonics
//   pROut
//      Output SH vector for Red.
//   pGOut
//      Output SH vector for Green
//   pBOut
//      Output SH vector for Blue        
//
//============================================================================

HRESULT WINAPI D3DXSHProjectCubeMap
    ( UINT uOrder, LPDIRECT3DCUBETEXTURE9 pCubeMap,
      FLOAT *pROut, FLOAT *pGOut, FLOAT *pBOut );


#ifdef __cplusplus
}
#endif


#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4201)
#endif

#endif // __D3DX9MATH_H__

///////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) Microsoft Corporation.  All Rights Reserved.
//
//  File:       d3dx9xof.h
//  Content:    D3DX .X File types and functions
//
///////////////////////////////////////////////////////////////////////////

#if !defined( __D3DX9XOF_H__ )
#define __D3DX9XOF_H__

#if defined( __cplusplus )
extern "C" {
#endif // defined( __cplusplus )

//----------------------------------------------------------------------------
// D3DXF_FILEFORMAT
//   This flag is used to specify what file type to use when saving to disk.
//   _BINARY, and _TEXT are mutually exclusive, while
//   _COMPRESSED is an optional setting that works with all file types.
//----------------------------------------------------------------------------
typedef DWORD D3DXF_FILEFORMAT;

#define D3DXF_FILEFORMAT_BINARY          0
#define D3DXF_FILEFORMAT_TEXT            1
#define D3DXF_FILEFORMAT_COMPRESSED      2

//----------------------------------------------------------------------------
// D3DXF_FILESAVEOPTIONS
//   This flag is used to specify where to save the file to. Each flag is
//   mutually exclusive, indicates the data location of the file, and also
//   chooses which additional data will specify the location.
//   _TOFILE is paired with a filename (LPCSTR)
//   _TOWFILE is paired with a filename (LPWSTR)
//----------------------------------------------------------------------------
typedef DWORD D3DXF_FILESAVEOPTIONS;

#define D3DXF_FILESAVE_TOFILE     0x00L
#define D3DXF_FILESAVE_TOWFILE    0x01L

//----------------------------------------------------------------------------
// D3DXF_FILELOADOPTIONS
//   This flag is used to specify where to load the file from. Each flag is
//   mutually exclusive, indicates the data location of the file, and also
//   chooses which additional data will specify the location.
//   _FROMFILE is paired with a filename (LPCSTR)
//   _FROMWFILE is paired with a filename (LPWSTR)
//   _FROMRESOURCE is paired with a (D3DXF_FILELOADRESOUCE*) description.
//   _FROMMEMORY is paired with a (D3DXF_FILELOADMEMORY*) description.
//----------------------------------------------------------------------------
typedef DWORD D3DXF_FILELOADOPTIONS;

#define D3DXF_FILELOAD_FROMFILE     0x00L
#define D3DXF_FILELOAD_FROMWFILE    0x01L
#define D3DXF_FILELOAD_FROMRESOURCE 0x02L
#define D3DXF_FILELOAD_FROMMEMORY   0x03L

//----------------------------------------------------------------------------
// D3DXF_FILELOADRESOURCE:
//----------------------------------------------------------------------------

typedef struct _D3DXF_FILELOADRESOURCE
{
    HMODULE hModule; // Desc
    LPCSTR lpName;  // Desc
    LPCSTR lpType;  // Desc
} D3DXF_FILELOADRESOURCE;

//----------------------------------------------------------------------------
// D3DXF_FILELOADMEMORY:
//----------------------------------------------------------------------------

typedef struct _D3DXF_FILELOADMEMORY
{
    LPCVOID lpMemory; // Desc
    SIZE_T  dSize;     // Desc
} D3DXF_FILELOADMEMORY;

#if defined( _WIN32 ) && !defined( _NO_COM )

// {cef08cf9-7b4f-4429-9624-2a690a933201}
DEFINE_GUID( IID_ID3DXFile, 
0xcef08cf9, 0x7b4f, 0x4429, 0x96, 0x24, 0x2a, 0x69, 0x0a, 0x93, 0x32, 0x01 );

// {cef08cfa-7b4f-4429-9624-2a690a933201}
DEFINE_GUID( IID_ID3DXFileSaveObject, 
0xcef08cfa, 0x7b4f, 0x4429, 0x96, 0x24, 0x2a, 0x69, 0x0a, 0x93, 0x32, 0x01 );

// {cef08cfb-7b4f-4429-9624-2a690a933201}
DEFINE_GUID( IID_ID3DXFileSaveData, 
0xcef08cfb, 0x7b4f, 0x4429, 0x96, 0x24, 0x2a, 0x69, 0x0a, 0x93, 0x32, 0x01 );

// {cef08cfc-7b4f-4429-9624-2a690a933201}
DEFINE_GUID( IID_ID3DXFileEnumObject, 
0xcef08cfc, 0x7b4f, 0x4429, 0x96, 0x24, 0x2a, 0x69, 0x0a, 0x93, 0x32, 0x01 );

// {cef08cfd-7b4f-4429-9624-2a690a933201}
DEFINE_GUID( IID_ID3DXFileData, 
0xcef08cfd, 0x7b4f, 0x4429, 0x96, 0x24, 0x2a, 0x69, 0x0a, 0x93, 0x32, 0x01 );

#endif // defined( _WIN32 ) && !defined( _NO_COM )

#if defined( __cplusplus )
#if !defined( DECLSPEC_UUID )
#if _MSC_VER >= 1100
#define DECLSPEC_UUID( x ) __declspec( uuid( x ) )
#else // !( _MSC_VER >= 1100 )
#define DECLSPEC_UUID( x )
#endif // !( _MSC_VER >= 1100 )
#endif // !defined( DECLSPEC_UUID )

interface DECLSPEC_UUID( "cef08cf9-7b4f-4429-9624-2a690a933201" )
    ID3DXFile;
interface DECLSPEC_UUID( "cef08cfa-7b4f-4429-9624-2a690a933201" )
    ID3DXFileSaveObject;
interface DECLSPEC_UUID( "cef08cfb-7b4f-4429-9624-2a690a933201" )
    ID3DXFileSaveData;
interface DECLSPEC_UUID( "cef08cfc-7b4f-4429-9624-2a690a933201" )
    ID3DXFileEnumObject;
interface DECLSPEC_UUID( "cef08cfd-7b4f-4429-9624-2a690a933201" )
    ID3DXFileData;

#if defined( _COM_SMARTPTR_TYPEDEF )
_COM_SMARTPTR_TYPEDEF( ID3DXFile,
    __uuidof( ID3DXFile ) );
_COM_SMARTPTR_TYPEDEF( ID3DXFileSaveObject,
    __uuidof( ID3DXFileSaveObject ) );
_COM_SMARTPTR_TYPEDEF( ID3DXFileSaveData,
    __uuidof( ID3DXFileSaveData ) );
_COM_SMARTPTR_TYPEDEF( ID3DXFileEnumObject,
    __uuidof( ID3DXFileEnumObject ) );
_COM_SMARTPTR_TYPEDEF( ID3DXFileData,
    __uuidof( ID3DXFileData ) );
#endif // defined( _COM_SMARTPTR_TYPEDEF )
#endif // defined( __cplusplus )

typedef interface ID3DXFile ID3DXFile;
typedef interface ID3DXFileSaveObject ID3DXFileSaveObject;
typedef interface ID3DXFileSaveData ID3DXFileSaveData;
typedef interface ID3DXFileEnumObject ID3DXFileEnumObject;
typedef interface ID3DXFileData ID3DXFileData;

//////////////////////////////////////////////////////////////////////////////
// ID3DXFile /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#undef INTERFACE
#define INTERFACE ID3DXFile

DECLARE_INTERFACE_( ID3DXFile, IUnknown )
{
    STDMETHOD( QueryInterface )( THIS_ REFIID, LPVOID* ) PURE;
    STDMETHOD_( ULONG, AddRef )( THIS ) PURE;
    STDMETHOD_( ULONG, Release )( THIS ) PURE;
    
    STDMETHOD( CreateEnumObject )( THIS_ LPCVOID, D3DXF_FILELOADOPTIONS,
        ID3DXFileEnumObject** ) PURE;
    STDMETHOD( CreateSaveObject )( THIS_ LPCVOID, D3DXF_FILESAVEOPTIONS,
        D3DXF_FILEFORMAT, ID3DXFileSaveObject** ) PURE;
    STDMETHOD( RegisterTemplates )( THIS_ LPCVOID, SIZE_T ) PURE;
    STDMETHOD( RegisterEnumTemplates )( THIS_ ID3DXFileEnumObject* ) PURE;
};

//////////////////////////////////////////////////////////////////////////////
// ID3DXFileSaveObject ///////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#undef INTERFACE
#define INTERFACE ID3DXFileSaveObject

DECLARE_INTERFACE_( ID3DXFileSaveObject, IUnknown )
{
    STDMETHOD( QueryInterface )( THIS_ REFIID, LPVOID* ) PURE;
    STDMETHOD_( ULONG, AddRef )( THIS ) PURE;
    STDMETHOD_( ULONG, Release )( THIS ) PURE;
    
    STDMETHOD( GetFile )( THIS_ ID3DXFile** ) PURE;
    STDMETHOD( AddDataObject )( THIS_ REFGUID, LPCSTR, CONST GUID*,
        SIZE_T, LPCVOID, ID3DXFileSaveData** ) PURE;
    STDMETHOD( Save )( THIS ) PURE;
};

//////////////////////////////////////////////////////////////////////////////
// ID3DXFileSaveData /////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#undef INTERFACE
#define INTERFACE ID3DXFileSaveData

DECLARE_INTERFACE_( ID3DXFileSaveData, IUnknown )
{
    STDMETHOD( QueryInterface )( THIS_ REFIID, LPVOID* ) PURE;
    STDMETHOD_( ULONG, AddRef )( THIS ) PURE;
    STDMETHOD_( ULONG, Release )( THIS ) PURE;
    
    STDMETHOD( GetSave )( THIS_ ID3DXFileSaveObject** ) PURE;
    STDMETHOD( GetName )( THIS_ LPSTR, SIZE_T* ) PURE;
    STDMETHOD( GetId )( THIS_ LPGUID ) PURE;
    STDMETHOD( GetType )( THIS_ GUID* ) PURE;
    STDMETHOD( AddDataObject )( THIS_ REFGUID, LPCSTR, CONST GUID*,
        SIZE_T, LPCVOID, ID3DXFileSaveData** ) PURE;
    STDMETHOD( AddDataReference )( THIS_ LPCSTR, CONST GUID* ) PURE;
};

//////////////////////////////////////////////////////////////////////////////
// ID3DXFileEnumObject ///////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#undef INTERFACE
#define INTERFACE ID3DXFileEnumObject

DECLARE_INTERFACE_( ID3DXFileEnumObject, IUnknown )
{
    STDMETHOD( QueryInterface )( THIS_ REFIID, LPVOID* ) PURE;
    STDMETHOD_( ULONG, AddRef )( THIS ) PURE;
    STDMETHOD_( ULONG, Release )( THIS ) PURE;
    
    STDMETHOD( GetFile )( THIS_ ID3DXFile** ) PURE;
    STDMETHOD( GetChildren )( THIS_ SIZE_T* ) PURE;
    STDMETHOD( GetChild )( THIS_ SIZE_T, ID3DXFileData** ) PURE;
    STDMETHOD( GetDataObjectById )( THIS_ REFGUID, ID3DXFileData** ) PURE;
    STDMETHOD( GetDataObjectByName )( THIS_ LPCSTR, ID3DXFileData** ) PURE;
};

//////////////////////////////////////////////////////////////////////////////
// ID3DXFileData /////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#undef INTERFACE
#define INTERFACE ID3DXFileData

DECLARE_INTERFACE_( ID3DXFileData, IUnknown )
{
    STDMETHOD( QueryInterface )( THIS_ REFIID, LPVOID* ) PURE;
    STDMETHOD_( ULONG, AddRef )( THIS ) PURE;
    STDMETHOD_( ULONG, Release )( THIS ) PURE;
    
    STDMETHOD( GetEnum )( THIS_ ID3DXFileEnumObject** ) PURE;
    STDMETHOD( GetName )( THIS_ LPSTR, SIZE_T* ) PURE;
    STDMETHOD( GetId )( THIS_ LPGUID ) PURE;
    STDMETHOD( Lock )( THIS_ SIZE_T*, LPCVOID* ) PURE;
    STDMETHOD( Unlock )( THIS ) PURE;
    STDMETHOD( GetType )( THIS_ GUID* ) PURE;
    STDMETHOD_( BOOL, IsReference )( THIS ) PURE;
    STDMETHOD( GetChildren )( THIS_ SIZE_T* ) PURE;
    STDMETHOD( GetChild )( THIS_ SIZE_T, ID3DXFileData** ) PURE;
};

STDAPI D3DXFileCreate( ID3DXFile** lplpDirectXFile );

/*
 * DirectX File errors.
 */

#define _FACD3DXF 0x876

#define D3DXFERR_BADOBJECT              MAKE_HRESULT( 1, _FACD3DXF, 900 )
#define D3DXFERR_BADVALUE               MAKE_HRESULT( 1, _FACD3DXF, 901 )
#define D3DXFERR_BADTYPE                MAKE_HRESULT( 1, _FACD3DXF, 902 )
#define D3DXFERR_NOTFOUND               MAKE_HRESULT( 1, _FACD3DXF, 903 )
#define D3DXFERR_NOTDONEYET             MAKE_HRESULT( 1, _FACD3DXF, 904 )
#define D3DXFERR_FILENOTFOUND           MAKE_HRESULT( 1, _FACD3DXF, 905 )
#define D3DXFERR_RESOURCENOTFOUND       MAKE_HRESULT( 1, _FACD3DXF, 906 )
#define D3DXFERR_BADRESOURCE            MAKE_HRESULT( 1, _FACD3DXF, 907 )
#define D3DXFERR_BADFILETYPE            MAKE_HRESULT( 1, _FACD3DXF, 908 )
#define D3DXFERR_BADFILEVERSION         MAKE_HRESULT( 1, _FACD3DXF, 909 )
#define D3DXFERR_BADFILEFLOATSIZE       MAKE_HRESULT( 1, _FACD3DXF, 910 )
#define D3DXFERR_BADFILE                MAKE_HRESULT( 1, _FACD3DXF, 911 )
#define D3DXFERR_PARSEERROR             MAKE_HRESULT( 1, _FACD3DXF, 912 )
#define D3DXFERR_BADARRAYSIZE           MAKE_HRESULT( 1, _FACD3DXF, 913 )
#define D3DXFERR_BADDATAREFERENCE       MAKE_HRESULT( 1, _FACD3DXF, 914 )
#define D3DXFERR_NOMOREOBJECTS          MAKE_HRESULT( 1, _FACD3DXF, 915 )
#define D3DXFERR_NOMOREDATA             MAKE_HRESULT( 1, _FACD3DXF, 916 )
#define D3DXFERR_BADCACHEFILE           MAKE_HRESULT( 1, _FACD3DXF, 917 )

/*
 * DirectX File object types.
 */

#ifndef WIN_TYPES
#define WIN_TYPES(itype, ptype) typedef interface itype *LP##ptype, **LPLP##ptype
#endif

WIN_TYPES(ID3DXFile,                 D3DXFILE);
WIN_TYPES(ID3DXFileEnumObject,       D3DXFILEENUMOBJECT);
WIN_TYPES(ID3DXFileSaveObject,       D3DXFILESAVEOBJECT);
WIN_TYPES(ID3DXFileData,             D3DXFILEDATA);
WIN_TYPES(ID3DXFileSaveData,         D3DXFILESAVEDATA);

#if defined( __cplusplus )
} // extern "C"
#endif // defined( __cplusplus )

#endif // !defined( __D3DX9XOF_H__ )





//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) Microsoft Corporation.  All Rights Reserved.
//
//  File:       d3dx9mesh.h
//  Content:    D3DX mesh types and functions
//
//////////////////////////////////////////////////////////////////////////////

#ifndef __D3DX9MESH_H__
#define __D3DX9MESH_H__

// {7ED943DD-52E8-40b5-A8D8-76685C406330}
DEFINE_GUID(IID_ID3DXBaseMesh, 
0x7ed943dd, 0x52e8, 0x40b5, 0xa8, 0xd8, 0x76, 0x68, 0x5c, 0x40, 0x63, 0x30);

// {4020E5C2-1403-4929-883F-E2E849FAC195}
DEFINE_GUID(IID_ID3DXMesh, 
0x4020e5c2, 0x1403, 0x4929, 0x88, 0x3f, 0xe2, 0xe8, 0x49, 0xfa, 0xc1, 0x95);

// {8875769A-D579-4088-AAEB-534D1AD84E96}
DEFINE_GUID(IID_ID3DXPMesh, 
0x8875769a, 0xd579, 0x4088, 0xaa, 0xeb, 0x53, 0x4d, 0x1a, 0xd8, 0x4e, 0x96);

// {667EA4C7-F1CD-4386-B523-7C0290B83CC5}
DEFINE_GUID(IID_ID3DXSPMesh, 
0x667ea4c7, 0xf1cd, 0x4386, 0xb5, 0x23, 0x7c, 0x2, 0x90, 0xb8, 0x3c, 0xc5);

// {11EAA540-F9A6-4d49-AE6A-E19221F70CC4}
DEFINE_GUID(IID_ID3DXSkinInfo, 
0x11eaa540, 0xf9a6, 0x4d49, 0xae, 0x6a, 0xe1, 0x92, 0x21, 0xf7, 0xc, 0xc4);

// {3CE6CC22-DBF2-44f4-894D-F9C34A337139}
DEFINE_GUID(IID_ID3DXPatchMesh, 
0x3ce6cc22, 0xdbf2, 0x44f4, 0x89, 0x4d, 0xf9, 0xc3, 0x4a, 0x33, 0x71, 0x39);

//patch mesh can be quads or tris
typedef enum _D3DXPATCHMESHTYPE {
    D3DXPATCHMESH_RECT   = 0x001,
    D3DXPATCHMESH_TRI    = 0x002,
    D3DXPATCHMESH_NPATCH = 0x003,

    D3DXPATCHMESH_FORCE_DWORD    = 0x7fffffff, /* force 32-bit size enum */
} D3DXPATCHMESHTYPE;

// Mesh options - lower 3 bytes only, upper byte used by _D3DXMESHOPT option flags
enum _D3DXMESH {
    D3DXMESH_32BIT                  = 0x001, // If set, then use 32 bit indices, if not set use 16 bit indices.
    D3DXMESH_DONOTCLIP              = 0x002, // Use D3DUSAGE_DONOTCLIP for VB & IB.
    D3DXMESH_POINTS                 = 0x004, // Use D3DUSAGE_POINTS for VB & IB. 
    D3DXMESH_RTPATCHES              = 0x008, // Use D3DUSAGE_RTPATCHES for VB & IB. 
    D3DXMESH_NPATCHES               = 0x4000,// Use D3DUSAGE_NPATCHES for VB & IB. 
    D3DXMESH_VB_SYSTEMMEM           = 0x010, // Use D3DPOOL_SYSTEMMEM for VB. Overrides D3DXMESH_MANAGEDVERTEXBUFFER
    D3DXMESH_VB_MANAGED             = 0x020, // Use D3DPOOL_MANAGED for VB. 
    D3DXMESH_VB_WRITEONLY           = 0x040, // Use D3DUSAGE_WRITEONLY for VB.
    D3DXMESH_VB_DYNAMIC             = 0x080, // Use D3DUSAGE_DYNAMIC for VB.
    D3DXMESH_VB_SOFTWAREPROCESSING = 0x8000, // Use D3DUSAGE_SOFTWAREPROCESSING for VB.
    D3DXMESH_IB_SYSTEMMEM           = 0x100, // Use D3DPOOL_SYSTEMMEM for IB. Overrides D3DXMESH_MANAGEDINDEXBUFFER
    D3DXMESH_IB_MANAGED             = 0x200, // Use D3DPOOL_MANAGED for IB.
    D3DXMESH_IB_WRITEONLY           = 0x400, // Use D3DUSAGE_WRITEONLY for IB.
    D3DXMESH_IB_DYNAMIC             = 0x800, // Use D3DUSAGE_DYNAMIC for IB.
    D3DXMESH_IB_SOFTWAREPROCESSING= 0x10000, // Use D3DUSAGE_SOFTWAREPROCESSING for IB.

    D3DXMESH_VB_SHARE               = 0x1000, // Valid for Clone* calls only, forces cloned mesh/pmesh to share vertex buffer

    D3DXMESH_USEHWONLY              = 0x2000, // Valid for ID3DXSkinInfo::ConvertToBlendedMesh

    // Helper options
    D3DXMESH_SYSTEMMEM              = 0x110, // D3DXMESH_VB_SYSTEMMEM | D3DXMESH_IB_SYSTEMMEM
    D3DXMESH_MANAGED                = 0x220, // D3DXMESH_VB_MANAGED | D3DXMESH_IB_MANAGED
    D3DXMESH_WRITEONLY              = 0x440, // D3DXMESH_VB_WRITEONLY | D3DXMESH_IB_WRITEONLY
    D3DXMESH_DYNAMIC                = 0x880, // D3DXMESH_VB_DYNAMIC | D3DXMESH_IB_DYNAMIC
    D3DXMESH_SOFTWAREPROCESSING   = 0x18000, // D3DXMESH_VB_SOFTWAREPROCESSING | D3DXMESH_IB_SOFTWAREPROCESSING

};

//patch mesh options
enum _D3DXPATCHMESH {
    D3DXPATCHMESH_DEFAULT = 000,
};
// option field values for specifying min value in D3DXGeneratePMesh and D3DXSimplifyMesh
enum _D3DXMESHSIMP
{
    D3DXMESHSIMP_VERTEX   = 0x1,
    D3DXMESHSIMP_FACE     = 0x2,

};

typedef enum _D3DXCLEANTYPE {
	D3DXCLEAN_BACKFACING	= 0x00000001,
	D3DXCLEAN_BOWTIES		= 0x00000002,
	
	// Helper options
	D3DXCLEAN_SKINNING		= D3DXCLEAN_BACKFACING,	// Bowtie cleaning modifies geometry and breaks skinning
	D3DXCLEAN_OPTIMIZATION	= D3DXCLEAN_BACKFACING,
	D3DXCLEAN_SIMPLIFICATION= D3DXCLEAN_BACKFACING | D3DXCLEAN_BOWTIES,	
} D3DXCLEANTYPE;

enum _MAX_FVF_DECL_SIZE
{
    MAX_FVF_DECL_SIZE = MAXD3DDECLLENGTH + 1 // +1 for END
};

typedef enum _D3DXTANGENT
{
    D3DXTANGENT_WRAP_U =                    0x01,
    D3DXTANGENT_WRAP_V =                    0x02,
    D3DXTANGENT_WRAP_UV =                   0x03,
    D3DXTANGENT_DONT_NORMALIZE_PARTIALS =   0x04,
    D3DXTANGENT_DONT_ORTHOGONALIZE =        0x08,
    D3DXTANGENT_ORTHOGONALIZE_FROM_V =      0x010,
    D3DXTANGENT_ORTHOGONALIZE_FROM_U =      0x020,
    D3DXTANGENT_WEIGHT_BY_AREA =            0x040,
    D3DXTANGENT_WEIGHT_EQUAL =              0x080,
    D3DXTANGENT_WIND_CW =                   0x0100,
    D3DXTANGENT_CALCULATE_NORMALS =         0x0200,
    D3DXTANGENT_GENERATE_IN_PLACE =         0x0400,
} D3DXTANGENT;

// D3DXIMT_WRAP_U means the texture wraps in the U direction
// D3DXIMT_WRAP_V means the texture wraps in the V direction
// D3DXIMT_WRAP_UV means the texture wraps in both directions
typedef enum _D3DXIMT
{
    D3DXIMT_WRAP_U =                    0x01,
    D3DXIMT_WRAP_V =                    0x02,
    D3DXIMT_WRAP_UV =                   0x03,
} D3DXIMT;

// These options are only valid for UVAtlasCreate and UVAtlasPartition, we may add more for UVAtlasPack if necessary
// D3DXUVATLAS_DEFAULT - Meshes with more than 25k faces go through fast, meshes with fewer than 25k faces go through quality
// D3DXUVATLAS_GEODESIC_FAST - Uses approximations to improve charting speed at the cost of added stretch or more charts.
// D3DXUVATLAS_GEODESIC_QUALITY - Provides better quality charts, but requires more time and memory than fast.
typedef enum _D3DXUVATLAS
{
    D3DXUVATLAS_DEFAULT               = 0x00,
    D3DXUVATLAS_GEODESIC_FAST         = 0x01,
    D3DXUVATLAS_GEODESIC_QUALITY      = 0x02,
} D3DXUVATLAS;

typedef struct ID3DXBaseMesh *LPD3DXBASEMESH;
typedef struct ID3DXMesh *LPD3DXMESH;
typedef struct ID3DXPMesh *LPD3DXPMESH;
typedef struct ID3DXSPMesh *LPD3DXSPMESH;
typedef struct ID3DXSkinInfo *LPD3DXSKININFO;
typedef struct ID3DXPatchMesh *LPD3DXPATCHMESH;
typedef interface ID3DXTextureGutterHelper *LPD3DXTEXTUREGUTTERHELPER;
typedef interface ID3DXPRTBuffer *LPD3DXPRTBUFFER;


typedef struct _D3DXATTRIBUTERANGE
{
    DWORD AttribId;
    DWORD FaceStart;
    DWORD FaceCount;
    DWORD VertexStart;
    DWORD VertexCount;
} D3DXATTRIBUTERANGE;

typedef D3DXATTRIBUTERANGE* LPD3DXATTRIBUTERANGE;

typedef struct _D3DXMATERIAL
{
    D3DMATERIAL9  MatD3D;
    LPSTR         pTextureFilename;
} D3DXMATERIAL;
typedef D3DXMATERIAL *LPD3DXMATERIAL;

typedef enum _D3DXEFFECTDEFAULTTYPE
{
    D3DXEDT_STRING = 0x1,       // pValue points to a null terminated ASCII string 
    D3DXEDT_FLOATS = 0x2,       // pValue points to an array of floats - number of floats is NumBytes / sizeof(float)
    D3DXEDT_DWORD  = 0x3,       // pValue points to a DWORD

    D3DXEDT_FORCEDWORD = 0x7fffffff
} D3DXEFFECTDEFAULTTYPE;

typedef struct _D3DXEFFECTDEFAULT
{
    LPSTR                 pParamName;
    D3DXEFFECTDEFAULTTYPE Type;           // type of the data pointed to by pValue
    DWORD                 NumBytes;       // size in bytes of the data pointed to by pValue
    LPVOID                pValue;         // data for the default of the effect
} D3DXEFFECTDEFAULT, *LPD3DXEFFECTDEFAULT;

typedef struct _D3DXEFFECTINSTANCE
{
    LPSTR               pEffectFilename;
    DWORD               NumDefaults;
    LPD3DXEFFECTDEFAULT pDefaults;
} D3DXEFFECTINSTANCE, *LPD3DXEFFECTINSTANCE;

typedef struct _D3DXATTRIBUTEWEIGHTS
{
    FLOAT Position;
    FLOAT Boundary;
    FLOAT Normal;
    FLOAT Diffuse;
    FLOAT Specular;
    FLOAT Texcoord[8];
    FLOAT Tangent;
    FLOAT Binormal;
} D3DXATTRIBUTEWEIGHTS, *LPD3DXATTRIBUTEWEIGHTS;

enum _D3DXWELDEPSILONSFLAGS
{
    D3DXWELDEPSILONS_WELDALL             = 0x1,  // weld all vertices marked by adjacency as being overlapping

    D3DXWELDEPSILONS_WELDPARTIALMATCHES  = 0x2,  // if a given vertex component is within epsilon, modify partial matched 
                                                    // vertices so that both components identical AND if all components "equal"
                                                    // remove one of the vertices
    D3DXWELDEPSILONS_DONOTREMOVEVERTICES = 0x4,  // instructs weld to only allow modifications to vertices and not removal
                                                    // ONLY valid if D3DXWELDEPSILONS_WELDPARTIALMATCHES is set
                                                    // useful to modify vertices to be equal, but not allow vertices to be removed

    D3DXWELDEPSILONS_DONOTSPLIT          = 0x8,  // instructs weld to specify the D3DXMESHOPT_DONOTSPLIT flag when doing an Optimize(ATTR_SORT)
                                                    // if this flag is not set, all vertices that are in separate attribute groups
                                                    // will remain split and not welded.  Setting this flag can slow down software vertex processing

};

typedef struct _D3DXWELDEPSILONS
{
    FLOAT Position;                 // NOTE: This does NOT replace the epsilon in GenerateAdjacency
                                            // in general, it should be the same value or greater than the one passed to GeneratedAdjacency
    FLOAT BlendWeights;
    FLOAT Normal;
    FLOAT PSize;
    FLOAT Specular;
    FLOAT Diffuse;
    FLOAT Texcoord[8];
    FLOAT Tangent;
    FLOAT Binormal;
    FLOAT TessFactor;
} D3DXWELDEPSILONS;

typedef D3DXWELDEPSILONS* LPD3DXWELDEPSILONS;


#undef INTERFACE
#define INTERFACE ID3DXBaseMesh

DECLARE_INTERFACE_(ID3DXBaseMesh, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // ID3DXBaseMesh
    STDMETHOD(DrawSubset)(THIS_ DWORD AttribId) PURE;
    STDMETHOD_(DWORD, GetNumFaces)(THIS) PURE;
    STDMETHOD_(DWORD, GetNumVertices)(THIS) PURE;
    STDMETHOD_(DWORD, GetFVF)(THIS) PURE;
    STDMETHOD(GetDeclaration)(THIS_ D3DVERTEXELEMENT9 Declaration[MAX_FVF_DECL_SIZE]) PURE;
    STDMETHOD_(DWORD, GetNumBytesPerVertex)(THIS) PURE;
    STDMETHOD_(DWORD, GetOptions)(THIS) PURE;
    STDMETHOD(GetDevice)(THIS_ LPDIRECT3DDEVICE9* ppDevice) PURE;
    STDMETHOD(CloneMeshFVF)(THIS_ DWORD Options, 
                DWORD FVF, LPDIRECT3DDEVICE9 pD3DDevice, LPD3DXMESH* ppCloneMesh) PURE;
    STDMETHOD(CloneMesh)(THIS_ DWORD Options, 
                CONST D3DVERTEXELEMENT9 *pDeclaration, LPDIRECT3DDEVICE9 pD3DDevice, LPD3DXMESH* ppCloneMesh) PURE;
    STDMETHOD(GetVertexBuffer)(THIS_ LPDIRECT3DVERTEXBUFFER9* ppVB) PURE;
    STDMETHOD(GetIndexBuffer)(THIS_ LPDIRECT3DINDEXBUFFER9* ppIB) PURE;
    STDMETHOD(LockVertexBuffer)(THIS_ DWORD Flags, LPVOID *ppData) PURE;
    STDMETHOD(UnlockVertexBuffer)(THIS) PURE;
    STDMETHOD(LockIndexBuffer)(THIS_ DWORD Flags, LPVOID *ppData) PURE;
    STDMETHOD(UnlockIndexBuffer)(THIS) PURE;
    STDMETHOD(GetAttributeTable)(
                THIS_ D3DXATTRIBUTERANGE *pAttribTable, DWORD* pAttribTableSize) PURE;

    STDMETHOD(ConvertPointRepsToAdjacency)(THIS_ CONST DWORD* pPRep, DWORD* pAdjacency) PURE;
    STDMETHOD(ConvertAdjacencyToPointReps)(THIS_ CONST DWORD* pAdjacency, DWORD* pPRep) PURE;
    STDMETHOD(GenerateAdjacency)(THIS_ FLOAT Epsilon, DWORD* pAdjacency) PURE;

    STDMETHOD(UpdateSemantics)(THIS_ D3DVERTEXELEMENT9 Declaration[MAX_FVF_DECL_SIZE]) PURE;
};


#undef INTERFACE
#define INTERFACE ID3DXMesh

DECLARE_INTERFACE_(ID3DXMesh, ID3DXBaseMesh)
{
    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // ID3DXBaseMesh
    STDMETHOD(DrawSubset)(THIS_ DWORD AttribId) PURE;
    STDMETHOD_(DWORD, GetNumFaces)(THIS) PURE;
    STDMETHOD_(DWORD, GetNumVertices)(THIS) PURE;
    STDMETHOD_(DWORD, GetFVF)(THIS) PURE;
    STDMETHOD(GetDeclaration)(THIS_ D3DVERTEXELEMENT9 Declaration[MAX_FVF_DECL_SIZE]) PURE;
    STDMETHOD_(DWORD, GetNumBytesPerVertex)(THIS) PURE;
    STDMETHOD_(DWORD, GetOptions)(THIS) PURE;
    STDMETHOD(GetDevice)(THIS_ LPDIRECT3DDEVICE9* ppDevice) PURE;
    STDMETHOD(CloneMeshFVF)(THIS_ DWORD Options, 
                DWORD FVF, LPDIRECT3DDEVICE9 pD3DDevice, LPD3DXMESH* ppCloneMesh) PURE;
    STDMETHOD(CloneMesh)(THIS_ DWORD Options, 
                CONST D3DVERTEXELEMENT9 *pDeclaration, LPDIRECT3DDEVICE9 pD3DDevice, LPD3DXMESH* ppCloneMesh) PURE;
    STDMETHOD(GetVertexBuffer)(THIS_ LPDIRECT3DVERTEXBUFFER9* ppVB) PURE;
    STDMETHOD(GetIndexBuffer)(THIS_ LPDIRECT3DINDEXBUFFER9* ppIB) PURE;
    STDMETHOD(LockVertexBuffer)(THIS_ DWORD Flags, LPVOID *ppData) PURE;
    STDMETHOD(UnlockVertexBuffer)(THIS) PURE;
    STDMETHOD(LockIndexBuffer)(THIS_ DWORD Flags, LPVOID *ppData) PURE;
    STDMETHOD(UnlockIndexBuffer)(THIS) PURE;
    STDMETHOD(GetAttributeTable)(
                THIS_ D3DXATTRIBUTERANGE *pAttribTable, DWORD* pAttribTableSize) PURE;

    STDMETHOD(ConvertPointRepsToAdjacency)(THIS_ CONST DWORD* pPRep, DWORD* pAdjacency) PURE;
    STDMETHOD(ConvertAdjacencyToPointReps)(THIS_ CONST DWORD* pAdjacency, DWORD* pPRep) PURE;
    STDMETHOD(GenerateAdjacency)(THIS_ FLOAT Epsilon, DWORD* pAdjacency) PURE;

    STDMETHOD(UpdateSemantics)(THIS_ D3DVERTEXELEMENT9 Declaration[MAX_FVF_DECL_SIZE]) PURE;

    // ID3DXMesh
    STDMETHOD(LockAttributeBuffer)(THIS_ DWORD Flags, DWORD** ppData) PURE;
    STDMETHOD(UnlockAttributeBuffer)(THIS) PURE;
    STDMETHOD(Optimize)(THIS_ DWORD Flags, CONST DWORD* pAdjacencyIn, DWORD* pAdjacencyOut, 
                     DWORD* pFaceRemap, LPD3DXBUFFER *ppVertexRemap,  
                     LPD3DXMESH* ppOptMesh) PURE;
    STDMETHOD(OptimizeInplace)(THIS_ DWORD Flags, CONST DWORD* pAdjacencyIn, DWORD* pAdjacencyOut, 
                     DWORD* pFaceRemap, LPD3DXBUFFER *ppVertexRemap) PURE;
    STDMETHOD(SetAttributeTable)(THIS_ CONST D3DXATTRIBUTERANGE *pAttribTable, DWORD cAttribTableSize) PURE;
};


#undef INTERFACE
#define INTERFACE ID3DXPMesh

DECLARE_INTERFACE_(ID3DXPMesh, ID3DXBaseMesh)
{
    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // ID3DXBaseMesh
    STDMETHOD(DrawSubset)(THIS_ DWORD AttribId) PURE;
    STDMETHOD_(DWORD, GetNumFaces)(THIS) PURE;
    STDMETHOD_(DWORD, GetNumVertices)(THIS) PURE;
    STDMETHOD_(DWORD, GetFVF)(THIS) PURE;
    STDMETHOD(GetDeclaration)(THIS_ D3DVERTEXELEMENT9 Declaration[MAX_FVF_DECL_SIZE]) PURE;
    STDMETHOD_(DWORD, GetNumBytesPerVertex)(THIS) PURE;
    STDMETHOD_(DWORD, GetOptions)(THIS) PURE;
    STDMETHOD(GetDevice)(THIS_ LPDIRECT3DDEVICE9* ppDevice) PURE;
    STDMETHOD(CloneMeshFVF)(THIS_ DWORD Options, 
                DWORD FVF, LPDIRECT3DDEVICE9 pD3DDevice, LPD3DXMESH* ppCloneMesh) PURE;
    STDMETHOD(CloneMesh)(THIS_ DWORD Options, 
                CONST D3DVERTEXELEMENT9 *pDeclaration, LPDIRECT3DDEVICE9 pD3DDevice, LPD3DXMESH* ppCloneMesh) PURE;
    STDMETHOD(GetVertexBuffer)(THIS_ LPDIRECT3DVERTEXBUFFER9* ppVB) PURE;
    STDMETHOD(GetIndexBuffer)(THIS_ LPDIRECT3DINDEXBUFFER9* ppIB) PURE;
    STDMETHOD(LockVertexBuffer)(THIS_ DWORD Flags, LPVOID *ppData) PURE;
    STDMETHOD(UnlockVertexBuffer)(THIS) PURE;
    STDMETHOD(LockIndexBuffer)(THIS_ DWORD Flags, LPVOID *ppData) PURE;
    STDMETHOD(UnlockIndexBuffer)(THIS) PURE;
    STDMETHOD(GetAttributeTable)(
                THIS_ D3DXATTRIBUTERANGE *pAttribTable, DWORD* pAttribTableSize) PURE;

    STDMETHOD(ConvertPointRepsToAdjacency)(THIS_ CONST DWORD* pPRep, DWORD* pAdjacency) PURE;
    STDMETHOD(ConvertAdjacencyToPointReps)(THIS_ CONST DWORD* pAdjacency, DWORD* pPRep) PURE;
    STDMETHOD(GenerateAdjacency)(THIS_ FLOAT Epsilon, DWORD* pAdjacency) PURE;

    STDMETHOD(UpdateSemantics)(THIS_ D3DVERTEXELEMENT9 Declaration[MAX_FVF_DECL_SIZE]) PURE;

    // ID3DXPMesh
    STDMETHOD(ClonePMeshFVF)(THIS_ DWORD Options, 
                DWORD FVF, LPDIRECT3DDEVICE9 pD3DDevice, LPD3DXPMESH* ppCloneMesh) PURE;
    STDMETHOD(ClonePMesh)(THIS_ DWORD Options, 
                CONST D3DVERTEXELEMENT9 *pDeclaration, LPDIRECT3DDEVICE9 pD3DDevice, LPD3DXPMESH* ppCloneMesh) PURE;
    STDMETHOD(SetNumFaces)(THIS_ DWORD Faces) PURE;
    STDMETHOD(SetNumVertices)(THIS_ DWORD Vertices) PURE;
    STDMETHOD_(DWORD, GetMaxFaces)(THIS) PURE;
    STDMETHOD_(DWORD, GetMinFaces)(THIS) PURE;
    STDMETHOD_(DWORD, GetMaxVertices)(THIS) PURE;
    STDMETHOD_(DWORD, GetMinVertices)(THIS) PURE;
    STDMETHOD(Save)(THIS_ IStream *pStream, CONST D3DXMATERIAL* pMaterials, CONST D3DXEFFECTINSTANCE* pEffectInstances, DWORD NumMaterials) PURE;

    STDMETHOD(Optimize)(THIS_ DWORD Flags, DWORD* pAdjacencyOut, 
                     DWORD* pFaceRemap, LPD3DXBUFFER *ppVertexRemap,  
                     LPD3DXMESH* ppOptMesh) PURE;

    STDMETHOD(OptimizeBaseLOD)(THIS_ DWORD Flags, DWORD* pFaceRemap) PURE;
    STDMETHOD(TrimByFaces)(THIS_ DWORD NewFacesMin, DWORD NewFacesMax, DWORD *rgiFaceRemap, DWORD *rgiVertRemap) PURE;
    STDMETHOD(TrimByVertices)(THIS_ DWORD NewVerticesMin, DWORD NewVerticesMax, DWORD *rgiFaceRemap, DWORD *rgiVertRemap) PURE;

    STDMETHOD(GetAdjacency)(THIS_ DWORD* pAdjacency) PURE;

    //  Used to generate the immediate "ancestor" for each vertex when it is removed by a vsplit.  Allows generation of geomorphs
    //     Vertex buffer must be equal to or greater than the maximum number of vertices in the pmesh
    STDMETHOD(GenerateVertexHistory)(THIS_ DWORD* pVertexHistory) PURE;
};


#undef INTERFACE
#define INTERFACE ID3DXSPMesh

DECLARE_INTERFACE_(ID3DXSPMesh, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // ID3DXSPMesh
    STDMETHOD_(DWORD, GetNumFaces)(THIS) PURE;
    STDMETHOD_(DWORD, GetNumVertices)(THIS) PURE;
    STDMETHOD_(DWORD, GetFVF)(THIS) PURE;
    STDMETHOD(GetDeclaration)(THIS_ D3DVERTEXELEMENT9 Declaration[MAX_FVF_DECL_SIZE]) PURE;
    STDMETHOD_(DWORD, GetOptions)(THIS) PURE;
    STDMETHOD(GetDevice)(THIS_ LPDIRECT3DDEVICE9* ppDevice) PURE;
    STDMETHOD(CloneMeshFVF)(THIS_ DWORD Options, 
                DWORD FVF, LPDIRECT3DDEVICE9 pD3DDevice, DWORD *pAdjacencyOut, DWORD *pVertexRemapOut, LPD3DXMESH* ppCloneMesh) PURE;
    STDMETHOD(CloneMesh)(THIS_ DWORD Options, 
                CONST D3DVERTEXELEMENT9 *pDeclaration, LPDIRECT3DDEVICE9 pD3DDevice, DWORD *pAdjacencyOut, DWORD *pVertexRemapOut, LPD3DXMESH* ppCloneMesh) PURE;
    STDMETHOD(ClonePMeshFVF)(THIS_ DWORD Options, 
                DWORD FVF, LPDIRECT3DDEVICE9 pD3DDevice, DWORD *pVertexRemapOut, FLOAT *pErrorsByFace, LPD3DXPMESH* ppCloneMesh) PURE;
    STDMETHOD(ClonePMesh)(THIS_ DWORD Options, 
                CONST D3DVERTEXELEMENT9 *pDeclaration, LPDIRECT3DDEVICE9 pD3DDevice, DWORD *pVertexRemapOut, FLOAT *pErrorsbyFace, LPD3DXPMESH* ppCloneMesh) PURE;
    STDMETHOD(ReduceFaces)(THIS_ DWORD Faces) PURE;
    STDMETHOD(ReduceVertices)(THIS_ DWORD Vertices) PURE;
    STDMETHOD_(DWORD, GetMaxFaces)(THIS) PURE;
    STDMETHOD_(DWORD, GetMaxVertices)(THIS) PURE;
    STDMETHOD(GetVertexAttributeWeights)(THIS_ LPD3DXATTRIBUTEWEIGHTS pVertexAttributeWeights) PURE;
    STDMETHOD(GetVertexWeights)(THIS_ FLOAT *pVertexWeights) PURE;
};

#define UNUSED16 (0xffff)
#define UNUSED32 (0xffffffff)

// ID3DXMesh::Optimize options - upper byte only, lower 3 bytes used from _D3DXMESH option flags
enum _D3DXMESHOPT {
    D3DXMESHOPT_COMPACT       = 0x01000000,
    D3DXMESHOPT_ATTRSORT      = 0x02000000,
    D3DXMESHOPT_VERTEXCACHE   = 0x04000000,
    D3DXMESHOPT_STRIPREORDER  = 0x08000000,
    D3DXMESHOPT_IGNOREVERTS   = 0x10000000,  // optimize faces only, don't touch vertices
    D3DXMESHOPT_DONOTSPLIT    = 0x20000000,  // do not split vertices shared between attribute groups when attribute sorting
    D3DXMESHOPT_DEVICEINDEPENDENT = 0x00400000,  // Only affects VCache.  uses a static known good cache size for all cards
                            
    // D3DXMESHOPT_SHAREVB has been removed, please use D3DXMESH_VB_SHARE instead

};

// Subset of the mesh that has the same attribute and bone combination.
// This subset can be rendered in a single draw call
typedef struct _D3DXBONECOMBINATION
{
    DWORD AttribId;
    DWORD FaceStart;
    DWORD FaceCount;
    DWORD VertexStart;
    DWORD VertexCount;
    DWORD* BoneId;
} D3DXBONECOMBINATION, *LPD3DXBONECOMBINATION;

// The following types of patch combinations are supported:
// Patch type   Basis       Degree
// Rect         Bezier      2,3,5
// Rect         B-Spline    2,3,5
// Rect         Catmull-Rom 3
// Tri          Bezier      2,3,5
// N-Patch      N/A         3

typedef struct _D3DXPATCHINFO
{
    D3DXPATCHMESHTYPE PatchType;
    D3DDEGREETYPE Degree;
    D3DBASISTYPE Basis;
} D3DXPATCHINFO, *LPD3DXPATCHINFO;

#undef INTERFACE
#define INTERFACE ID3DXPatchMesh

DECLARE_INTERFACE_(ID3DXPatchMesh, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // ID3DXPatchMesh

    // Return creation parameters
    STDMETHOD_(DWORD, GetNumPatches)(THIS) PURE;
    STDMETHOD_(DWORD, GetNumVertices)(THIS) PURE;
    STDMETHOD(GetDeclaration)(THIS_ D3DVERTEXELEMENT9 Declaration[MAX_FVF_DECL_SIZE]) PURE;
    STDMETHOD_(DWORD, GetControlVerticesPerPatch)(THIS) PURE;
    STDMETHOD_(DWORD, GetOptions)(THIS) PURE;
    STDMETHOD(GetDevice)(THIS_ LPDIRECT3DDEVICE9 *ppDevice) PURE;
    STDMETHOD(GetPatchInfo)(THIS_ LPD3DXPATCHINFO PatchInfo) PURE;

    // Control mesh access    
    STDMETHOD(GetVertexBuffer)(THIS_ LPDIRECT3DVERTEXBUFFER9* ppVB) PURE;
    STDMETHOD(GetIndexBuffer)(THIS_ LPDIRECT3DINDEXBUFFER9* ppIB) PURE;
    STDMETHOD(LockVertexBuffer)(THIS_ DWORD flags, LPVOID *ppData) PURE;
    STDMETHOD(UnlockVertexBuffer)(THIS) PURE;
    STDMETHOD(LockIndexBuffer)(THIS_ DWORD flags, LPVOID *ppData) PURE;
    STDMETHOD(UnlockIndexBuffer)(THIS) PURE;
    STDMETHOD(LockAttributeBuffer)(THIS_ DWORD flags, DWORD** ppData) PURE;
    STDMETHOD(UnlockAttributeBuffer)(THIS) PURE;

    // This function returns the size of the tessellated mesh given a tessellation level.
    // This assumes uniform tessellation. For adaptive tessellation the Adaptive parameter must
    // be set to TRUE and TessellationLevel should be the max tessellation.
    // This will result in the max mesh size necessary for adaptive tessellation.    
    STDMETHOD(GetTessSize)(THIS_ FLOAT fTessLevel,DWORD Adaptive, DWORD *NumTriangles,DWORD *NumVertices) PURE;
    
    //GenerateAdjacency determines which patches are adjacent with provided tolerance
    //this information is used internally to optimize tessellation
    STDMETHOD(GenerateAdjacency)(THIS_ FLOAT Tolerance) PURE;
    
    //CloneMesh Creates a new patchmesh with the specified decl, and converts the vertex buffer
    //to the new decl. Entries in the new decl which are new are set to 0. If the current mesh
    //has adjacency, the new mesh will also have adjacency
    STDMETHOD(CloneMesh)(THIS_ DWORD Options, CONST D3DVERTEXELEMENT9 *pDecl, LPD3DXPATCHMESH *pMesh) PURE;
    
    // Optimizes the patchmesh for efficient tessellation. This function is designed
    // to perform one time optimization for patch meshes that need to be tessellated
    // repeatedly by calling the Tessellate() method. The optimization performed is
    // independent of the actual tessellation level used.
    // Currently Flags is unused.
    // If vertices are changed, Optimize must be called again
    STDMETHOD(Optimize)(THIS_ DWORD flags) PURE;

    //gets and sets displacement parameters
    //displacement maps can only be 2D textures MIP-MAPPING is ignored for non adapative tessellation
    STDMETHOD(SetDisplaceParam)(THIS_ LPDIRECT3DBASETEXTURE9 Texture,
                              D3DTEXTUREFILTERTYPE MinFilter,
                              D3DTEXTUREFILTERTYPE MagFilter,
                              D3DTEXTUREFILTERTYPE MipFilter,
                              D3DTEXTUREADDRESS Wrap,
                              DWORD dwLODBias) PURE;

    STDMETHOD(GetDisplaceParam)(THIS_ LPDIRECT3DBASETEXTURE9 *Texture,
                                D3DTEXTUREFILTERTYPE *MinFilter,
                                D3DTEXTUREFILTERTYPE *MagFilter,
                                D3DTEXTUREFILTERTYPE *MipFilter,
                                D3DTEXTUREADDRESS *Wrap,
                                DWORD *dwLODBias) PURE;
        
    // Performs the uniform tessellation based on the tessellation level. 
    // This function will perform more efficiently if the patch mesh has been optimized using the Optimize() call.
    STDMETHOD(Tessellate)(THIS_ FLOAT fTessLevel,LPD3DXMESH pMesh) PURE;

    // Performs adaptive tessellation based on the Z based adaptive tessellation criterion.
    // pTrans specifies a 4D vector that is dotted with the vertices to get the per vertex
    // adaptive tessellation amount. Each edge is tessellated to the average of the criterion
    // at the 2 vertices it connects.
    // MaxTessLevel specifies the upper limit for adaptive tesselation.
    // This function will perform more efficiently if the patch mesh has been optimized using the Optimize() call.
    STDMETHOD(TessellateAdaptive)(THIS_ 
        CONST D3DXVECTOR4 *pTrans,
        DWORD dwMaxTessLevel, 
        DWORD dwMinTessLevel,
        LPD3DXMESH pMesh) PURE;

};

#undef INTERFACE
#define INTERFACE ID3DXSkinInfo

DECLARE_INTERFACE_(ID3DXSkinInfo, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // Specify the which vertices do each bones influence and by how much
    STDMETHOD(SetBoneInfluence)(THIS_ DWORD bone, DWORD numInfluences, CONST DWORD* vertices, CONST FLOAT* weights) PURE;
	STDMETHOD(SetBoneVertexInfluence)(THIS_ DWORD boneNum, DWORD influenceNum, float weight) PURE;
    STDMETHOD_(DWORD, GetNumBoneInfluences)(THIS_ DWORD bone) PURE;
	STDMETHOD(GetBoneInfluence)(THIS_ DWORD bone, DWORD* vertices, FLOAT* weights) PURE;
	STDMETHOD(GetBoneVertexInfluence)(THIS_ DWORD boneNum, DWORD influenceNum, float *pWeight, DWORD *pVertexNum) PURE;
    STDMETHOD(GetMaxVertexInfluences)(THIS_ DWORD* maxVertexInfluences) PURE;
    STDMETHOD_(DWORD, GetNumBones)(THIS) PURE;
	STDMETHOD(FindBoneVertexInfluenceIndex)(THIS_ DWORD boneNum, DWORD vertexNum, DWORD *pInfluenceIndex) PURE;

    // This gets the max face influences based on a triangle mesh with the specified index buffer
    STDMETHOD(GetMaxFaceInfluences)(THIS_ LPDIRECT3DINDEXBUFFER9 pIB, DWORD NumFaces, DWORD* maxFaceInfluences) PURE;
    
    // Set min bone influence. Bone influences that are smaller than this are ignored
    STDMETHOD(SetMinBoneInfluence)(THIS_ FLOAT MinInfl) PURE;
    // Get min bone influence. 
    STDMETHOD_(FLOAT, GetMinBoneInfluence)(THIS) PURE;
    
    // Bone names are returned by D3DXLoadSkinMeshFromXof. They are not used by any other method of this object
    STDMETHOD(SetBoneName)(THIS_ DWORD Bone, LPCSTR pName) PURE; // pName is copied to an internal string buffer
    STDMETHOD_(LPCSTR, GetBoneName)(THIS_ DWORD Bone) PURE; // A pointer to an internal string buffer is returned. Do not free this.
    
    // Bone offset matrices are returned by D3DXLoadSkinMeshFromXof. They are not used by any other method of this object
    STDMETHOD(SetBoneOffsetMatrix)(THIS_ DWORD Bone, CONST D3DXMATRIX *pBoneTransform) PURE; // pBoneTransform is copied to an internal buffer
    STDMETHOD_(LPD3DXMATRIX, GetBoneOffsetMatrix)(THIS_ DWORD Bone) PURE; // A pointer to an internal matrix is returned. Do not free this.
    
    // Clone a skin info object
    STDMETHOD(Clone)(THIS_ LPD3DXSKININFO* ppSkinInfo) PURE;
    
    // Update bone influence information to match vertices after they are reordered. This should be called 
    // if the target vertex buffer has been reordered externally.
    STDMETHOD(Remap)(THIS_ DWORD NumVertices, DWORD* pVertexRemap) PURE;

    // These methods enable the modification of the vertex layout of the vertices that will be skinned
    STDMETHOD(SetFVF)(THIS_ DWORD FVF) PURE;
    STDMETHOD(SetDeclaration)(THIS_ CONST D3DVERTEXELEMENT9 *pDeclaration) PURE;
    STDMETHOD_(DWORD, GetFVF)(THIS) PURE;
    STDMETHOD(GetDeclaration)(THIS_ D3DVERTEXELEMENT9 Declaration[MAX_FVF_DECL_SIZE]) PURE;

    // Apply SW skinning based on current pose matrices to the target vertices.
    STDMETHOD(UpdateSkinnedMesh)(THIS_ 
        CONST D3DXMATRIX* pBoneTransforms, 
        CONST D3DXMATRIX* pBoneInvTransposeTransforms, 
        LPCVOID pVerticesSrc, 
        PVOID pVerticesDst) PURE;

    // Takes a mesh and returns a new mesh with per vertex blend weights and a bone combination
    // table that describes which bones affect which subsets of the mesh
    STDMETHOD(ConvertToBlendedMesh)(THIS_ 
        LPD3DXMESH pMesh,
        DWORD Options, 
        CONST DWORD *pAdjacencyIn, 
        LPDWORD pAdjacencyOut,
        DWORD* pFaceRemap, 
        LPD3DXBUFFER *ppVertexRemap, 
        DWORD* pMaxFaceInfl,
        DWORD* pNumBoneCombinations, 
        LPD3DXBUFFER* ppBoneCombinationTable, 
        LPD3DXMESH* ppMesh) PURE;

    // Takes a mesh and returns a new mesh with per vertex blend weights and indices 
    // and a bone combination table that describes which bones palettes affect which subsets of the mesh
    STDMETHOD(ConvertToIndexedBlendedMesh)(THIS_ 
        LPD3DXMESH pMesh,
        DWORD Options, 
        DWORD paletteSize, 
        CONST DWORD *pAdjacencyIn, 
        LPDWORD pAdjacencyOut, 
        DWORD* pFaceRemap, 
        LPD3DXBUFFER *ppVertexRemap, 
        DWORD* pMaxVertexInfl,
        DWORD* pNumBoneCombinations, 
        LPD3DXBUFFER* ppBoneCombinationTable, 
        LPD3DXMESH* ppMesh) PURE;
};

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


HRESULT WINAPI 
    D3DXCreateMesh(
        DWORD NumFaces, 
        DWORD NumVertices, 
        DWORD Options, 
        CONST D3DVERTEXELEMENT9 *pDeclaration, 
        LPDIRECT3DDEVICE9 pD3DDevice, 
        LPD3DXMESH* ppMesh);

HRESULT WINAPI 
    D3DXCreateMeshFVF(
        DWORD NumFaces, 
        DWORD NumVertices, 
        DWORD Options, 
        DWORD FVF, 
        LPDIRECT3DDEVICE9 pD3DDevice, 
        LPD3DXMESH* ppMesh);

HRESULT WINAPI 
    D3DXCreateSPMesh(
        LPD3DXMESH pMesh, 
        CONST DWORD* pAdjacency, 
        CONST D3DXATTRIBUTEWEIGHTS *pVertexAttributeWeights,
        CONST FLOAT *pVertexWeights,
        LPD3DXSPMESH* ppSMesh);

// clean a mesh up for simplification, try to make manifold
HRESULT WINAPI
    D3DXCleanMesh(
    D3DXCLEANTYPE CleanType,
    LPD3DXMESH pMeshIn,
    CONST DWORD* pAdjacencyIn,
    LPD3DXMESH* ppMeshOut,
    DWORD* pAdjacencyOut,
    LPD3DXBUFFER* ppErrorsAndWarnings);

HRESULT WINAPI
    D3DXValidMesh(
    LPD3DXMESH pMeshIn,
    CONST DWORD* pAdjacency,
    LPD3DXBUFFER* ppErrorsAndWarnings);

HRESULT WINAPI 
    D3DXGeneratePMesh(
        LPD3DXMESH pMesh, 
        CONST DWORD* pAdjacency, 
        CONST D3DXATTRIBUTEWEIGHTS *pVertexAttributeWeights,
        CONST FLOAT *pVertexWeights,
        DWORD MinValue, 
        DWORD Options, 
        LPD3DXPMESH* ppPMesh);

HRESULT WINAPI 
    D3DXSimplifyMesh(
        LPD3DXMESH pMesh, 
        CONST DWORD* pAdjacency, 
        CONST D3DXATTRIBUTEWEIGHTS *pVertexAttributeWeights,
        CONST FLOAT *pVertexWeights,
        DWORD MinValue, 
        DWORD Options, 
        LPD3DXMESH* ppMesh);

HRESULT WINAPI 
    D3DXComputeBoundingSphere(
        CONST D3DXVECTOR3 *pFirstPosition,  // pointer to first position
        DWORD NumVertices, 
        DWORD dwStride,                     // count in bytes to subsequent position vectors
        D3DXVECTOR3 *pCenter, 
        FLOAT *pRadius);

HRESULT WINAPI 
    D3DXComputeBoundingBox(
        CONST D3DXVECTOR3 *pFirstPosition,  // pointer to first position
        DWORD NumVertices, 
        DWORD dwStride,                     // count in bytes to subsequent position vectors
        D3DXVECTOR3 *pMin, 
        D3DXVECTOR3 *pMax);

HRESULT WINAPI 
    D3DXComputeNormals(
        LPD3DXBASEMESH pMesh,
        CONST DWORD *pAdjacency);

HRESULT WINAPI 
    D3DXCreateBuffer(
        DWORD NumBytes, 
        LPD3DXBUFFER *ppBuffer);


HRESULT WINAPI
    D3DXLoadMeshFromXA(
        LPCSTR pFilename, 
        DWORD Options, 
        LPDIRECT3DDEVICE9 pD3DDevice, 
        LPD3DXBUFFER *ppAdjacency,
        LPD3DXBUFFER *ppMaterials, 
        LPD3DXBUFFER *ppEffectInstances, 
        DWORD *pNumMaterials,
        LPD3DXMESH *ppMesh);

HRESULT WINAPI
    D3DXLoadMeshFromXW(
        LPCWSTR pFilename, 
        DWORD Options, 
        LPDIRECT3DDEVICE9 pD3DDevice, 
        LPD3DXBUFFER *ppAdjacency,
        LPD3DXBUFFER *ppMaterials, 
        LPD3DXBUFFER *ppEffectInstances, 
        DWORD *pNumMaterials,
        LPD3DXMESH *ppMesh);

#ifdef UNICODE
#define D3DXLoadMeshFromX D3DXLoadMeshFromXW
#else
#define D3DXLoadMeshFromX D3DXLoadMeshFromXA
#endif

HRESULT WINAPI 
    D3DXLoadMeshFromXInMemory(
        LPCVOID Memory,
        DWORD SizeOfMemory,
        DWORD Options, 
        LPDIRECT3DDEVICE9 pD3DDevice, 
        LPD3DXBUFFER *ppAdjacency,
        LPD3DXBUFFER *ppMaterials, 
        LPD3DXBUFFER *ppEffectInstances, 
        DWORD *pNumMaterials,
        LPD3DXMESH *ppMesh);

HRESULT WINAPI 
    D3DXLoadMeshFromXResource(
        HMODULE Module,
        LPCSTR Name,
        LPCSTR Type,
        DWORD Options, 
        LPDIRECT3DDEVICE9 pD3DDevice, 
        LPD3DXBUFFER *ppAdjacency,
        LPD3DXBUFFER *ppMaterials, 
        LPD3DXBUFFER *ppEffectInstances, 
        DWORD *pNumMaterials,
        LPD3DXMESH *ppMesh);

HRESULT WINAPI 
    D3DXSaveMeshToXA(
        LPCSTR pFilename,
        LPD3DXMESH pMesh,
        CONST DWORD* pAdjacency,
        CONST D3DXMATERIAL* pMaterials,
        CONST D3DXEFFECTINSTANCE* pEffectInstances, 
        DWORD NumMaterials,
        DWORD Format
        );

HRESULT WINAPI 
    D3DXSaveMeshToXW(
        LPCWSTR pFilename,
        LPD3DXMESH pMesh,
        CONST DWORD* pAdjacency,
        CONST D3DXMATERIAL* pMaterials,
        CONST D3DXEFFECTINSTANCE* pEffectInstances, 
        DWORD NumMaterials,
        DWORD Format
        );
        
#ifdef UNICODE
#define D3DXSaveMeshToX D3DXSaveMeshToXW
#else
#define D3DXSaveMeshToX D3DXSaveMeshToXA
#endif
        

HRESULT WINAPI 
    D3DXCreatePMeshFromStream(
        IStream *pStream, 
        DWORD Options,
        LPDIRECT3DDEVICE9 pD3DDevice, 
        LPD3DXBUFFER *ppMaterials,
        LPD3DXBUFFER *ppEffectInstances, 
        DWORD* pNumMaterials,
        LPD3DXPMESH *ppPMesh);

// Creates a skin info object based on the number of vertices, number of bones, and a declaration describing the vertex layout of the target vertices
// The bone names and initial bone transforms are not filled in the skin info object by this method.
HRESULT WINAPI
    D3DXCreateSkinInfo(
        DWORD NumVertices,
        CONST D3DVERTEXELEMENT9 *pDeclaration, 
        DWORD NumBones,
        LPD3DXSKININFO* ppSkinInfo);
        
// Creates a skin info object based on the number of vertices, number of bones, and a FVF describing the vertex layout of the target vertices
// The bone names and initial bone transforms are not filled in the skin info object by this method.
HRESULT WINAPI
    D3DXCreateSkinInfoFVF(
        DWORD NumVertices,
        DWORD FVF,
        DWORD NumBones,
        LPD3DXSKININFO* ppSkinInfo);
        
#ifdef __cplusplus
}

extern "C" {
#endif //__cplusplus

HRESULT WINAPI 
    D3DXLoadMeshFromXof(
        LPD3DXFILEDATA pxofMesh, 
        DWORD Options, 
        LPDIRECT3DDEVICE9 pD3DDevice, 
        LPD3DXBUFFER *ppAdjacency,
        LPD3DXBUFFER *ppMaterials, 
        LPD3DXBUFFER *ppEffectInstances, 
        DWORD *pNumMaterials,
        LPD3DXMESH *ppMesh);

// This similar to D3DXLoadMeshFromXof, except also returns skinning info if present in the file
// If skinning info is not present, ppSkinInfo will be NULL     
HRESULT WINAPI
    D3DXLoadSkinMeshFromXof(
        LPD3DXFILEDATA pxofMesh, 
        DWORD Options,
        LPDIRECT3DDEVICE9 pD3DDevice,
        LPD3DXBUFFER* ppAdjacency,
        LPD3DXBUFFER* ppMaterials,
        LPD3DXBUFFER *ppEffectInstances, 
        DWORD *pMatOut,
        LPD3DXSKININFO* ppSkinInfo,
        LPD3DXMESH* ppMesh);


// The inverse of D3DXConvertTo{Indexed}BlendedMesh() functions. It figures out the skinning info from
// the mesh and the bone combination table and populates a skin info object with that data. The bone
// names and initial bone transforms are not filled in the skin info object by this method. This works
// with either a non-indexed or indexed blended mesh. It examines the FVF or declarator of the mesh to
// determine what type it is.
HRESULT WINAPI
    D3DXCreateSkinInfoFromBlendedMesh(
        LPD3DXBASEMESH pMesh,
        DWORD NumBones,
        CONST D3DXBONECOMBINATION *pBoneCombinationTable,
        LPD3DXSKININFO* ppSkinInfo);
        
HRESULT WINAPI
    D3DXTessellateNPatches(
        LPD3DXMESH pMeshIn,             
        CONST DWORD* pAdjacencyIn,             
        FLOAT NumSegs,                    
        BOOL  QuadraticInterpNormals,     // if false use linear intrep for normals, if true use quadratic
        LPD3DXMESH *ppMeshOut,
        LPD3DXBUFFER *ppAdjacencyOut);


//generates implied outputdecl from input decl
//the decl generated from this should be used to generate the output decl for
//the tessellator subroutines. 

HRESULT WINAPI
    D3DXGenerateOutputDecl(
        D3DVERTEXELEMENT9 *pOutput,
        CONST D3DVERTEXELEMENT9 *pInput);

//loads patches from an XFileData
//since an X file can have up to 6 different patch meshes in it,
//returns them in an array - pNumPatches will contain the number of
//meshes in the actual file. 
HRESULT WINAPI
    D3DXLoadPatchMeshFromXof(
        LPD3DXFILEDATA pXofObjMesh,
        DWORD Options,
        LPDIRECT3DDEVICE9 pD3DDevice,
        LPD3DXBUFFER *ppMaterials,
        LPD3DXBUFFER *ppEffectInstances, 
        PDWORD pNumMaterials,
        LPD3DXPATCHMESH *ppMesh);

//computes the size a single rect patch.
HRESULT WINAPI
    D3DXRectPatchSize(
        CONST FLOAT *pfNumSegs, //segments for each edge (4)
        DWORD *pdwTriangles,    //output number of triangles
        DWORD *pdwVertices);    //output number of vertices

//computes the size of a single triangle patch      
HRESULT WINAPI
    D3DXTriPatchSize(
        CONST FLOAT *pfNumSegs, //segments for each edge (3)    
        DWORD *pdwTriangles,    //output number of triangles
        DWORD *pdwVertices);    //output number of vertices


//tessellates a patch into a created mesh
//similar to D3D RT patch
HRESULT WINAPI
    D3DXTessellateRectPatch(
        LPDIRECT3DVERTEXBUFFER9 pVB,
        CONST FLOAT *pNumSegs,
        CONST D3DVERTEXELEMENT9 *pdwInDecl,
        CONST D3DRECTPATCH_INFO *pRectPatchInfo,
        LPD3DXMESH pMesh);


HRESULT WINAPI
    D3DXTessellateTriPatch(
      LPDIRECT3DVERTEXBUFFER9 pVB,
      CONST FLOAT *pNumSegs,
      CONST D3DVERTEXELEMENT9 *pInDecl,
      CONST D3DTRIPATCH_INFO *pTriPatchInfo,
      LPD3DXMESH pMesh);



//creates an NPatch PatchMesh from a D3DXMESH 
HRESULT WINAPI
    D3DXCreateNPatchMesh(
        LPD3DXMESH pMeshSysMem,
        LPD3DXPATCHMESH *pPatchMesh);

      
//creates a patch mesh
HRESULT WINAPI
    D3DXCreatePatchMesh(
        CONST D3DXPATCHINFO *pInfo,     //patch type
        DWORD dwNumPatches,             //number of patches
        DWORD dwNumVertices,            //number of control vertices
        DWORD dwOptions,                //options 
        CONST D3DVERTEXELEMENT9 *pDecl, //format of control vertices
        LPDIRECT3DDEVICE9 pD3DDevice, 
        LPD3DXPATCHMESH *pPatchMesh);

        
//returns the number of degenerates in a patch mesh -
//text output put in string.
HRESULT WINAPI
    D3DXValidPatchMesh(LPD3DXPATCHMESH pMesh,
                        DWORD *dwcDegenerateVertices,
                        DWORD *dwcDegeneratePatches,
                        LPD3DXBUFFER *ppErrorsAndWarnings);

UINT WINAPI
    D3DXGetFVFVertexSize(DWORD FVF);

UINT WINAPI 
    D3DXGetDeclVertexSize(CONST D3DVERTEXELEMENT9 *pDecl,DWORD Stream);

UINT WINAPI 
    D3DXGetDeclLength(CONST D3DVERTEXELEMENT9 *pDecl);

HRESULT WINAPI
    D3DXDeclaratorFromFVF(
        DWORD FVF,
        D3DVERTEXELEMENT9 pDeclarator[MAX_FVF_DECL_SIZE]);

HRESULT WINAPI
    D3DXFVFFromDeclarator(
        CONST D3DVERTEXELEMENT9 *pDeclarator,
        DWORD *pFVF);

HRESULT WINAPI 
    D3DXWeldVertices(
        LPD3DXMESH pMesh,         
        DWORD Flags,
        CONST D3DXWELDEPSILONS *pEpsilons,                 
        CONST DWORD *pAdjacencyIn, 
        DWORD *pAdjacencyOut,
        DWORD *pFaceRemap, 
        LPD3DXBUFFER *ppVertexRemap);

typedef struct _D3DXINTERSECTINFO
{
    DWORD FaceIndex;                // index of face intersected
    FLOAT U;                        // Barycentric Hit Coordinates    
    FLOAT V;                        // Barycentric Hit Coordinates
    FLOAT Dist;                     // Ray-Intersection Parameter Distance
} D3DXINTERSECTINFO, *LPD3DXINTERSECTINFO;


HRESULT WINAPI
    D3DXIntersect(
        LPD3DXBASEMESH pMesh,
        CONST D3DXVECTOR3 *pRayPos,
        CONST D3DXVECTOR3 *pRayDir, 
        BOOL    *pHit,              // True if any faces were intersected
        DWORD   *pFaceIndex,        // index of closest face intersected
        FLOAT   *pU,                // Barycentric Hit Coordinates    
        FLOAT   *pV,                // Barycentric Hit Coordinates
        FLOAT   *pDist,             // Ray-Intersection Parameter Distance
        LPD3DXBUFFER *ppAllHits,    // Array of D3DXINTERSECTINFOs for all hits (not just closest) 
        DWORD   *pCountOfHits);     // Number of entries in AllHits array

HRESULT WINAPI
    D3DXIntersectSubset(
        LPD3DXBASEMESH pMesh,
        DWORD AttribId,
        CONST D3DXVECTOR3 *pRayPos,
        CONST D3DXVECTOR3 *pRayDir, 
        BOOL    *pHit,              // True if any faces were intersected
        DWORD   *pFaceIndex,        // index of closest face intersected
        FLOAT   *pU,                // Barycentric Hit Coordinates    
        FLOAT   *pV,                // Barycentric Hit Coordinates
        FLOAT   *pDist,             // Ray-Intersection Parameter Distance
        LPD3DXBUFFER *ppAllHits,    // Array of D3DXINTERSECTINFOs for all hits (not just closest) 
        DWORD   *pCountOfHits);     // Number of entries in AllHits array


HRESULT WINAPI D3DXSplitMesh
    (
    LPD3DXMESH pMeshIn,         
    CONST DWORD *pAdjacencyIn, 
    CONST DWORD MaxSize,
    CONST DWORD Options,
    DWORD *pMeshesOut,
    LPD3DXBUFFER *ppMeshArrayOut,
    LPD3DXBUFFER *ppAdjacencyArrayOut,
    LPD3DXBUFFER *ppFaceRemapArrayOut,
    LPD3DXBUFFER *ppVertRemapArrayOut
    );

BOOL WINAPI D3DXIntersectTri 
(
    CONST D3DXVECTOR3 *p0,           // Triangle vertex 0 position
    CONST D3DXVECTOR3 *p1,           // Triangle vertex 1 position
    CONST D3DXVECTOR3 *p2,           // Triangle vertex 2 position
    CONST D3DXVECTOR3 *pRayPos,      // Ray origin
    CONST D3DXVECTOR3 *pRayDir,      // Ray direction
    FLOAT *pU,                       // Barycentric Hit Coordinates
    FLOAT *pV,                       // Barycentric Hit Coordinates
    FLOAT *pDist);                   // Ray-Intersection Parameter Distance

BOOL WINAPI
    D3DXSphereBoundProbe(
        CONST D3DXVECTOR3 *pCenter,
        FLOAT Radius,
        CONST D3DXVECTOR3 *pRayPosition,
        CONST D3DXVECTOR3 *pRayDirection);

BOOL WINAPI 
    D3DXBoxBoundProbe(
        CONST D3DXVECTOR3 *pMin, 
        CONST D3DXVECTOR3 *pMax,
        CONST D3DXVECTOR3 *pRayPosition,
        CONST D3DXVECTOR3 *pRayDirection);


HRESULT WINAPI D3DXComputeTangentFrame(ID3DXMesh *pMesh,
                                       DWORD dwOptions);

HRESULT WINAPI D3DXComputeTangentFrameEx(ID3DXMesh *pMesh,
                                         DWORD dwTextureInSemantic,
                                         DWORD dwTextureInIndex,
                                         DWORD dwUPartialOutSemantic,
                                         DWORD dwUPartialOutIndex,
                                         DWORD dwVPartialOutSemantic,
                                         DWORD dwVPartialOutIndex,
                                         DWORD dwNormalOutSemantic,
                                         DWORD dwNormalOutIndex,
                                         DWORD dwOptions,
                                         CONST DWORD *pdwAdjacency,
                                         FLOAT fPartialEdgeThreshold,
                                         FLOAT fSingularPointThreshold,
                                         FLOAT fNormalEdgeThreshold,
                                         ID3DXMesh **ppMeshOut,
                                         ID3DXBuffer **ppVertexMapping);


//D3DXComputeTangent
//
//Computes the Tangent vectors for the TexStage texture coordinates
//and places the results in the TANGENT[TangentIndex] specified in the meshes' DECL
//puts the binorm in BINORM[BinormIndex] also specified in the decl.
//
//If neither the binorm or the tangnet are in the meshes declaration,
//the function will fail. 
//
//If a tangent or Binorm field is in the Decl, but the user does not
//wish D3DXComputeTangent to replace them, then D3DX_DEFAULT specified
//in the TangentIndex or BinormIndex will cause it to ignore the specified 
//semantic.
//
//Wrap should be specified if the texture coordinates wrap.

HRESULT WINAPI D3DXComputeTangent(LPD3DXMESH Mesh,
                                 DWORD TexStage,
                                 DWORD TangentIndex,
                                 DWORD BinormIndex,
                                 DWORD Wrap,
                                 CONST DWORD *pAdjacency);

//============================================================================
//
// UVAtlas apis
//
//============================================================================
typedef HRESULT (WINAPI *LPD3DXUVATLASCB)(FLOAT fPercentDone,  LPVOID lpUserContext);

// This function creates atlases for meshes. There are two modes of operation,
// either based on the number of charts, or the maximum allowed stretch. If the
// maximum allowed stretch is 0, then each triangle will likely be in its own
// chart.

//
// The parameters are as follows:
//  pMesh - Input mesh to calculate an atlas for. This must have a position
//          channel and at least a 2-d texture channel.
//  uMaxChartNumber - The maximum number of charts required for the atlas.
//                    If this is 0, it will be parameterized based solely on
//                    stretch.
//  fMaxStretch - The maximum amount of stretch, if 0, no stretching is allowed,
//                if 1, then any amount of stretching is allowed.
//  uWidth - The width of the texture the atlas will be used on.
//  uHeight - The height of the texture the atlas will be used on.
//  fGutter - The minimum distance, in texels between two charts on the atlas.
//            this gets scaled by the width, so if fGutter is 2.5, and it is
//            used on a 512x512 texture, then the minimum distance will be
//            2.5 / 512 in u-v space.
//  dwTextureIndex - Specifies which texture coordinate to write to in the
//                   output mesh (which is cloned from the input mesh). Useful
//                   if your vertex has multiple texture coordinates.
//  pdwAdjacency - a pointer to an array with 3 DWORDs per face, indicating
//                 which triangles are adjacent to each other.
//  pdwFalseEdgeAdjacency - a pointer to an array with 3 DWORDS per face, indicating
//                          at each face, whether an edge is a false edge or not (using
//                          the same ordering as the adjacency data structure). If this
//                          is NULL, then it is assumed that there are no false edges. If
//                          not NULL, then a non-false edge is indicated by -1 and a false
//                          edge is indicated by any other value (it is not required, but
//                          it may be useful for the caller to use the original adjacency
//                          value). This allows you to parameterize a mesh of quads, and
//                          the edges down the middle of each quad will not be cut when
//                          parameterizing the mesh.
//  pfIMTArray - a pointer to an array with 3 FLOATs per face, describing the
//               integrated metric tensor for that face. This lets you control
//               the way this triangle may be stretched in the atlas. The IMT
//               passed in will be 3 floats (a,b,c) and specify a symmetric
//               matrix (a b) that, given a vector (s,t), specifies the 
//                      (b c)
//               distance between a vector v1 and a vector v2 = v1 + (s,t) as
//               sqrt((s, t) * M * (s, t)^T).
//               In other words, this lets one specify the magnitude of the
//               stretch in an arbitrary direction in u-v space. For example
//               if a = b = c = 1, then this scales the vector (1,1) by 2, and
//               the vector (1,-1) by 0. Note that this is multiplying the edge
//               length by the square of the matrix, so if you want the face to
//               stretch to twice its
//               size with no shearing, the IMT value should be (2, 0, 2), which
//               is just the identity matrix times 2.
//               Note that this assumes you have an orientation for the triangle
//               in some 2-D space. For D3DXUVAtlas, this space is created by
//               letting S be the direction from the first to the second
//               vertex, and T be the cross product between the normal and S.
//               
//  pStatusCallback - Since the atlas creation process can be very CPU intensive,
//                    this allows the programmer to specify a function to be called
//                    periodically, similarly to how it is done in the PRT simulation
//                    engine.
//  fCallbackFrequency - This lets you specify how often the callback will be
//                       called. A decent default should be 0.0001f.
//  pUserContext - a void pointer to be passed back to the callback function
//  dwOptions - A combination of flags in the D3DXUVATLAS enum
//  ppMeshOut - A pointer to a location to store a pointer for the newly created
//              mesh.
//  ppFacePartitioning - A pointer to a location to store a pointer for an array,
//                       one DWORD per face, giving the final partitioning
//                       created by the atlasing algorithm.
//  ppVertexRemapArray - A pointer to a location to store a pointer for an array,
//                       one DWORD per vertex, giving the vertex it was copied
//                       from, if any vertices needed to be split.
//  pfMaxStretchOut - A location to store the maximum stretch resulting from the
//                    atlasing algorithm.
//  puNumChartsOut - A location to store the number of charts created, or if the
//                   maximum number of charts was too low, this gives the minimum
//                    number of charts needed to create an atlas.

HRESULT WINAPI D3DXUVAtlasCreate(LPD3DXMESH pMesh,
                                 UINT uMaxChartNumber,
                                 FLOAT fMaxStretch,
                                 UINT uWidth,
                                 UINT uHeight,
                                 FLOAT fGutter,
                                 DWORD dwTextureIndex,
                                 CONST DWORD *pdwAdjacency,
                                 CONST DWORD *pdwFalseEdgeAdjacency,
                                 CONST FLOAT *pfIMTArray,
                                 LPD3DXUVATLASCB pStatusCallback,
                                 FLOAT fCallbackFrequency,
                                 LPVOID pUserContext,
                                 DWORD dwOptions,
                                 LPD3DXMESH *ppMeshOut,
                                 LPD3DXBUFFER *ppFacePartitioning,
                                 LPD3DXBUFFER *ppVertexRemapArray,
                                 FLOAT *pfMaxStretchOut,
                                 UINT *puNumChartsOut);

// This has the same exact arguments as Create, except that it does not perform the
// final packing step. This method allows one to get a partitioning out, and possibly
// modify it before sending it to be repacked. Note that if you change the
// partitioning, you'll also need to calculate new texture coordinates for any faces
// that have switched charts.
//
// The partition result adjacency output parameter is meant to be passed to the
// UVAtlasPack function, this adjacency cuts edges that are between adjacent
// charts, and also can include cuts inside of a chart in order to make it
// equivalent to a disc. For example:
//
// _______
// | ___ |
// | |_| |
// |_____|
//
// In order to make this equivalent to a disc, we would need to add a cut, and it
// Would end up looking like:
// _______
// | ___ |
// | |_|_|
// |_____|
//
// The resulting partition adjacency parameter cannot be NULL, because it is
// required for the packing step.



HRESULT WINAPI D3DXUVAtlasPartition(LPD3DXMESH pMesh,
                                    UINT uMaxChartNumber,
                                    FLOAT fMaxStretch,
                                    DWORD dwTextureIndex,
                                    CONST DWORD *pdwAdjacency,
                                    CONST DWORD *pdwFalseEdgeAdjacency,
                                    CONST FLOAT *pfIMTArray,
                                    LPD3DXUVATLASCB pStatusCallback,
                                    FLOAT fCallbackFrequency,
                                    LPVOID pUserContext,
                                    DWORD dwOptions,
                                    LPD3DXMESH *ppMeshOut,
                                    LPD3DXBUFFER *ppFacePartitioning,
                                    LPD3DXBUFFER *ppVertexRemapArray,
                                    LPD3DXBUFFER *ppPartitionResultAdjacency,
                                    FLOAT *pfMaxStretchOut,
                                    UINT *puNumChartsOut);

// This takes the face partitioning result from Partition and packs it into an
// atlas of the given size. pdwPartitionResultAdjacency should be derived from
// the adjacency returned from the partition step. This value cannot be NULL
// because Pack needs to know where charts were cut in the partition step in
// order to find the edges of each chart.
// The options parameter is currently reserved.
HRESULT WINAPI D3DXUVAtlasPack(ID3DXMesh *pMesh,
                               UINT uWidth,
                               UINT uHeight,
                               FLOAT fGutter,
                               DWORD dwTextureIndex,
                               CONST DWORD *pdwPartitionResultAdjacency,
                               LPD3DXUVATLASCB pStatusCallback,
                               FLOAT fCallbackFrequency,
                               LPVOID pUserContext,
                               DWORD dwOptions,
                               LPD3DXBUFFER pFacePartitioning);


//============================================================================
//
// IMT Calculation apis
//
// These functions all compute the Integrated Metric Tensor for use in the
// UVAtlas API. They all calculate the IMT with respect to the canonical
// triangle, where the coordinate system is set up so that the u axis goes
// from vertex 0 to 1 and the v axis is N x u. So, for example, the second
// vertex's canonical uv coordinates are (d,0) where d is the distance between
// vertices 0 and 1. This way the IMT does not depend on the parameterization
// of the mesh, and if the signal over the surface doesn't change, then
// the IMT doesn't need to be recalculated.
//============================================================================

// This callback is used by D3DXComputeIMTFromSignal.
//
// uv               - The texture coordinate for the vertex.
// uPrimitiveID     - Face ID of the triangle on which to compute the signal.
// uSignalDimension - The number of floats to store in pfSignalOut.
// pUserData        - The pUserData pointer passed in to ComputeIMTFromSignal.
// pfSignalOut      - A pointer to where to store the signal data.
typedef HRESULT (WINAPI* LPD3DXIMTSIGNALCALLBACK)
    (CONST D3DXVECTOR2 *uv,
     UINT uPrimitiveID,
     UINT uSignalDimension,
     VOID *pUserData,
     FLOAT *pfSignalOut);

// This function is used to calculate the IMT from per vertex data. It sets
// up a linear system over the triangle, solves for the jacobian J, then
// constructs the IMT from that (J^TJ).
// This function allows you to calculate the IMT based off of any value in a
// mesh (color, normal, etc) by specifying the correct stride of the array.
// The IMT computed will cause areas of the mesh that have similar values to
// take up less space in the texture.
//
// pMesh            - The mesh to calculate the IMT for.
// pVertexSignal    - A float array of size uSignalStride * v, where v is the
//                    number of vertices in the mesh.
// uSignalDimension - How many floats per vertex to use in calculating the IMT.
// uSignalStride    - The number of bytes per vertex in the array. This must be
//                    a multiple of sizeof(float)
// ppIMTData        - Where to store the buffer holding the IMT data

HRESULT WINAPI D3DXComputeIMTFromPerVertexSignal (
    LPD3DXMESH pMesh,
    CONST FLOAT *pfVertexSignal, // uSignalDimension floats per vertex
    UINT uSignalDimension,
    UINT uSignalStride,         // stride of signal in bytes
    DWORD dwOptions,            // reserved for future use
    LPD3DXUVATLASCB pStatusCallback,
    LPVOID pUserContext,
    LPD3DXBUFFER *ppIMTData);

// This function is used to calculate the IMT from data that varies over the
// surface of the mesh (generally at a higher frequency than vertex data).
// This function requires the mesh to already be parameterized (so it already
// has texture coordinates). It allows the user to define a signal arbitrarily
// over the surface of the mesh.
//
// pMesh            - The mesh to calculate the IMT for.
// dwTextureIndex   - This describes which set of texture coordinates in the
//                    mesh to use.
// uSignalDimension - How many components there are in the signal.
// fMaxUVDistance   - The subdivision will continue until the distance between
//                    all vertices is at most fMaxUVDistance.
// dwOptions        - reserved for future use
// pSignalCallback  - The callback to use to get the signal.
// pUserData        - A pointer that will be passed in to the callback.
// ppIMTData        - Where to store the buffer holding the IMT data
HRESULT WINAPI D3DXComputeIMTFromSignal(
    LPD3DXMESH pMesh,
    DWORD dwTextureIndex,
    UINT uSignalDimension,
    FLOAT fMaxUVDistance,
    DWORD dwOptions, // reserved for future use
    LPD3DXIMTSIGNALCALLBACK pSignalCallback,
    VOID *pUserData,
    LPD3DXUVATLASCB pStatusCallback,
    LPVOID pUserContext,
    LPD3DXBUFFER *ppIMTData);

// This function is used to calculate the IMT from texture data. Given a texture
// that maps over the surface of the mesh, the algorithm computes the IMT for
// each face. This will cause large areas that are very similar to take up less
// room when parameterized with UVAtlas. The texture is assumed to be
// interpolated over the mesh bilinearly.
//
// pMesh            - The mesh to calculate the IMT for.
// pTexture         - The texture to load data from.
// dwTextureIndex   - This describes which set of texture coordinates in the
//                    mesh to use.
// dwOptions        - Combination of one or more D3DXIMT flags.
// ppIMTData        - Where to store the buffer holding the IMT data
HRESULT WINAPI D3DXComputeIMTFromTexture (
    LPD3DXMESH pMesh,
    LPDIRECT3DTEXTURE9 pTexture,
    DWORD dwTextureIndex,
    DWORD dwOptions,
    LPD3DXUVATLASCB pStatusCallback,
    LPVOID pUserContext,
    LPD3DXBUFFER *ppIMTData);

// This function is very similar to ComputeIMTFromTexture, but it uses a
// float array to pass in the data, and it can calculate higher dimensional
// values than 4.
//
// pMesh            - The mesh to calculate the IMT for.
// dwTextureIndex   - This describes which set of texture coordinates in the
//                    mesh to use.
// pfFloatArray     - a pointer to a float array of size
//                    uWidth*uHeight*uComponents
// uWidth           - The width of the texture
// uHeight          - The height of the texture
// uSignalDimension - The number of floats per texel in the signal
// uComponents      - The number of floats in each texel
// dwOptions        - Combination of one or more D3DXIMT flags
// ppIMTData        - Where to store the buffer holding the IMT data
HRESULT WINAPI D3DXComputeIMTFromPerTexelSignal(
    LPD3DXMESH pMesh,
    DWORD dwTextureIndex,
    FLOAT *pfTexelSignal,
    UINT uWidth,
    UINT uHeight,
    UINT uSignalDimension,
    UINT uComponents,
    DWORD dwOptions,
    LPD3DXUVATLASCB pStatusCallback,
    LPVOID pUserContext,
    LPD3DXBUFFER *ppIMTData);

HRESULT WINAPI
    D3DXConvertMeshSubsetToSingleStrip(
        LPD3DXBASEMESH MeshIn,
        DWORD AttribId,
        DWORD IBOptions,
        LPDIRECT3DINDEXBUFFER9 *ppIndexBuffer,
        DWORD *pNumIndices);

HRESULT WINAPI
    D3DXConvertMeshSubsetToStrips(
        LPD3DXBASEMESH MeshIn,
        DWORD AttribId,
        DWORD IBOptions,
        LPDIRECT3DINDEXBUFFER9 *ppIndexBuffer,
        DWORD *pNumIndices,
        LPD3DXBUFFER *ppStripLengths,
        DWORD *pNumStrips);

        
//============================================================================
//
//  D3DXOptimizeFaces:
//  --------------------
//  Generate a face remapping for a triangle list that more effectively utilizes
//    vertex caches.  This optimization is identical to the one provided
//    by ID3DXMesh::Optimize with the hardware independent option enabled.
//
//  Parameters:
//   pbIndices
//      Triangle list indices to use for generating a vertex ordering
//   NumFaces
//      Number of faces in the triangle list
//   NumVertices
//      Number of vertices referenced by the triangle list
//   b32BitIndices
//      TRUE if indices are 32 bit, FALSE if indices are 16 bit
//   pFaceRemap
//      Destination buffer to store face ordering
//      The number stored for a given element is where in the new ordering
//        the face will have come from.  See ID3DXMesh::Optimize for more info.
//
//============================================================================
HRESULT WINAPI
    D3DXOptimizeFaces(
        LPCVOID pbIndices, 
        UINT cFaces, 
        UINT cVertices, 
        BOOL b32BitIndices, 
        DWORD* pFaceRemap);
        
//============================================================================
//
//  D3DXOptimizeVertices:
//  --------------------
//  Generate a vertex remapping to optimize for in order use of vertices for 
//    a given set of indices.  This is commonly used after applying the face
//    remap generated by D3DXOptimizeFaces
//
//  Parameters:
//   pbIndices
//      Triangle list indices to use for generating a vertex ordering
//   NumFaces
//      Number of faces in the triangle list
//   NumVertices
//      Number of vertices referenced by the triangle list
//   b32BitIndices
//      TRUE if indices are 32 bit, FALSE if indices are 16 bit
//   pVertexRemap
//      Destination buffer to store vertex ordering
//      The number stored for a given element is where in the new ordering
//        the vertex will have come from.  See ID3DXMesh::Optimize for more info.
//
//============================================================================
HRESULT WINAPI
    D3DXOptimizeVertices(
        LPCVOID pbIndices, 
        UINT cFaces, 
        UINT cVertices, 
        BOOL b32BitIndices, 
        DWORD* pVertexRemap);

#ifdef __cplusplus
}
#endif //__cplusplus


//===========================================================================
//
//  Data structures for Spherical Harmonic Precomputation
//
//
//============================================================================

typedef enum _D3DXSHCOMPRESSQUALITYTYPE {
    D3DXSHCQUAL_FASTLOWQUALITY  = 1,
    D3DXSHCQUAL_SLOWHIGHQUALITY = 2,
    D3DXSHCQUAL_FORCE_DWORD     = 0x7fffffff
} D3DXSHCOMPRESSQUALITYTYPE;

typedef enum _D3DXSHGPUSIMOPT {
    D3DXSHGPUSIMOPT_SHADOWRES256  = 1,
    D3DXSHGPUSIMOPT_SHADOWRES512  = 0,
    D3DXSHGPUSIMOPT_SHADOWRES1024 = 2,
    D3DXSHGPUSIMOPT_SHADOWRES2048 = 3,

    D3DXSHGPUSIMOPT_HIGHQUALITY = 4,    
    
    D3DXSHGPUSIMOPT_FORCE_DWORD = 0x7fffffff
} D3DXSHGPUSIMOPT;

// for all properties that are colors the luminance is computed
// if the simulator is run with a single channel using the following
// formula:  R * 0.2125 + G * 0.7154 + B * 0.0721

typedef struct _D3DXSHMATERIAL {
    D3DCOLORVALUE Diffuse;  // Diffuse albedo of the surface.  (Ignored if object is a Mirror)
    BOOL          bMirror;  // Must be set to FALSE.  bMirror == TRUE not currently supported
    BOOL          bSubSurf; // true if the object does subsurface scattering - can't do this and be a mirror

    // subsurface scattering parameters 
    FLOAT         RelativeIndexOfRefraction;
    D3DCOLORVALUE Absorption;
    D3DCOLORVALUE ReducedScattering;

} D3DXSHMATERIAL;

// allocated in D3DXSHPRTCompSplitMeshSC
// vertices are duplicated into multiple super clusters but
// only have a valid status in one super cluster (fill in the rest)

typedef struct _D3DXSHPRTSPLITMESHVERTDATA {
    UINT  uVertRemap;   // vertex in original mesh this corresponds to
    UINT  uSubCluster;  // cluster index relative to super cluster
    UCHAR ucVertStatus; // 1 if vertex has valid data, 0 if it is "fill"
} D3DXSHPRTSPLITMESHVERTDATA;

// used in D3DXSHPRTCompSplitMeshSC
// information for each super cluster that maps into face/vert arrays

typedef struct _D3DXSHPRTSPLITMESHCLUSTERDATA {
    UINT uVertStart;     // initial index into remapped vertex array
    UINT uVertLength;    // number of vertices in this super cluster
    
    UINT uFaceStart;     // initial index into face array
    UINT uFaceLength;    // number of faces in this super cluster
    
    UINT uClusterStart;  // initial index into cluster array
    UINT uClusterLength; // number of clusters in this super cluster
} D3DXSHPRTSPLITMESHCLUSTERDATA;

// call back function for simulator
// return S_OK to keep running the simulator - anything else represents
// failure and the simulator will abort.

typedef HRESULT (WINAPI *LPD3DXSHPRTSIMCB)(float fPercentDone,  LPVOID lpUserContext);

// interfaces for PRT buffers/simulator

// GUIDs
// {F1827E47-00A8-49cd-908C-9D11955F8728}
DEFINE_GUID(IID_ID3DXPRTBuffer, 
0xf1827e47, 0xa8, 0x49cd, 0x90, 0x8c, 0x9d, 0x11, 0x95, 0x5f, 0x87, 0x28);

// {A758D465-FE8D-45ad-9CF0-D01E56266A07}
DEFINE_GUID(IID_ID3DXPRTCompBuffer, 
0xa758d465, 0xfe8d, 0x45ad, 0x9c, 0xf0, 0xd0, 0x1e, 0x56, 0x26, 0x6a, 0x7);

// {838F01EC-9729-4527-AADB-DF70ADE7FEA9}
DEFINE_GUID(IID_ID3DXTextureGutterHelper, 
0x838f01ec, 0x9729, 0x4527, 0xaa, 0xdb, 0xdf, 0x70, 0xad, 0xe7, 0xfe, 0xa9);

// {683A4278-CD5F-4d24-90AD-C4E1B6855D53}
DEFINE_GUID(IID_ID3DXPRTEngine, 
0x683a4278, 0xcd5f, 0x4d24, 0x90, 0xad, 0xc4, 0xe1, 0xb6, 0x85, 0x5d, 0x53);

// interface definitions

typedef interface ID3DXTextureGutterHelper ID3DXTextureGutterHelper;
typedef interface ID3DXPRTBuffer ID3DXPRTBuffer;

#undef INTERFACE
#define INTERFACE ID3DXPRTBuffer

// Buffer interface - contains "NumSamples" samples
// each sample in memory is stored as NumCoeffs scalars per channel (1 or 3)
// Same interface is used for both Vertex and Pixel PRT buffers

DECLARE_INTERFACE_(ID3DXPRTBuffer, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // ID3DXPRTBuffer
    STDMETHOD_(UINT, GetNumSamples)(THIS) PURE;
    STDMETHOD_(UINT, GetNumCoeffs)(THIS) PURE;
    STDMETHOD_(UINT, GetNumChannels)(THIS) PURE;

    STDMETHOD_(BOOL, IsTexture)(THIS) PURE;
    STDMETHOD_(UINT, GetWidth)(THIS) PURE;
    STDMETHOD_(UINT, GetHeight)(THIS) PURE;

    // changes the number of samples allocated in the buffer
    STDMETHOD(Resize)(THIS_ UINT NewSize) PURE;

    // ppData will point to the memory location where sample Start begins
    // pointer is valid for at least NumSamples samples
    STDMETHOD(LockBuffer)(THIS_ UINT Start, UINT NumSamples, FLOAT **ppData) PURE;
    STDMETHOD(UnlockBuffer)(THIS) PURE;

    // every scalar in buffer is multiplied by Scale
    STDMETHOD(ScaleBuffer)(THIS_ FLOAT Scale) PURE;
    
    // every scalar contains the sum of this and pBuffers values
    // pBuffer must have the same storage class/dimensions 
    STDMETHOD(AddBuffer)(THIS_ LPD3DXPRTBUFFER pBuffer) PURE;

    // GutterHelper (described below) will fill in the gutter
    // regions of a texture by interpolating "internal" values
    STDMETHOD(AttachGH)(THIS_ LPD3DXTEXTUREGUTTERHELPER) PURE;
    STDMETHOD(ReleaseGH)(THIS) PURE;
    
    // Evaluates attached gutter helper on the contents of this buffer
    STDMETHOD(EvalGH)(THIS) PURE;

    // extracts a given channel into texture pTexture
    // NumCoefficients starting from StartCoefficient are copied
    STDMETHOD(ExtractTexture)(THIS_ UINT Channel, UINT StartCoefficient, 
                              UINT NumCoefficients, LPDIRECT3DTEXTURE9 pTexture) PURE;

    // extracts NumCoefficients coefficients into mesh - only applicable on single channel
    // buffers, otherwise just lockbuffer and copy data.  With SHPRT data NumCoefficients 
    // should be Order^2
    STDMETHOD(ExtractToMesh)(THIS_ UINT NumCoefficients, D3DDECLUSAGE Usage, UINT UsageIndexStart,
                             LPD3DXMESH pScene) PURE;

};

typedef interface ID3DXPRTCompBuffer ID3DXPRTCompBuffer;
typedef interface ID3DXPRTCompBuffer *LPD3DXPRTCOMPBUFFER;

#undef INTERFACE
#define INTERFACE ID3DXPRTCompBuffer

// compressed buffers stored a compressed version of a PRTBuffer

DECLARE_INTERFACE_(ID3DXPRTCompBuffer, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // ID3DPRTCompBuffer

    // NumCoeffs and NumChannels are properties of input buffer
    STDMETHOD_(UINT, GetNumSamples)(THIS) PURE;
    STDMETHOD_(UINT, GetNumCoeffs)(THIS) PURE;
    STDMETHOD_(UINT, GetNumChannels)(THIS) PURE;

    STDMETHOD_(BOOL, IsTexture)(THIS) PURE;
    STDMETHOD_(UINT, GetWidth)(THIS) PURE;
    STDMETHOD_(UINT, GetHeight)(THIS) PURE;

    // number of clusters, and PCA vectors per-cluster
    STDMETHOD_(UINT, GetNumClusters)(THIS) PURE;
    STDMETHOD_(UINT, GetNumPCA)(THIS) PURE;

    // normalizes PCA weights so that they are between [-1,1]
    // basis vectors are modified to reflect this
    STDMETHOD(NormalizeData)(THIS) PURE;

    // copies basis vectors for cluster "Cluster" into pClusterBasis
    // (NumPCA+1)*NumCoeffs*NumChannels floats
    STDMETHOD(ExtractBasis)(THIS_ UINT Cluster, FLOAT *pClusterBasis) PURE;
    
    // UINT per sample - which cluster it belongs to
    STDMETHOD(ExtractClusterIDs)(THIS_ UINT *pClusterIDs) PURE;
    
    // copies NumExtract PCA projection coefficients starting at StartPCA
    // into pPCACoefficients - NumSamples*NumExtract floats copied
    STDMETHOD(ExtractPCA)(THIS_ UINT StartPCA, UINT NumExtract, FLOAT *pPCACoefficients) PURE;

    // copies NumPCA projection coefficients starting at StartPCA
    // into pTexture - should be able to cope with signed formats
    STDMETHOD(ExtractTexture)(THIS_ UINT StartPCA, UINT NumpPCA, 
                              LPDIRECT3DTEXTURE9 pTexture) PURE;
                              
    // copies NumPCA projection coefficients into mesh pScene
    // Usage is D3DDECLUSAGE where coefficients are to be stored
    // UsageIndexStart is starting index
    STDMETHOD(ExtractToMesh)(THIS_ UINT NumPCA, D3DDECLUSAGE Usage, UINT UsageIndexStart,
                             LPD3DXMESH pScene) PURE;
};


#undef INTERFACE
#define INTERFACE ID3DXTextureGutterHelper

// ID3DXTextureGutterHelper will build and manage
// "gutter" regions in a texture - this will allow for
// bi-linear interpolation to not have artifacts when rendering
// It generates a map (in texture space) where each texel
// is in one of 3 states:
//   0  Invalid - not used at all
//   1  Inside triangle
//   2  Gutter texel
//   4  represents a gutter texel that will be computed during PRT
// For each Inside/Gutter texel it stores the face it
// belongs to and barycentric coordinates for the 1st two
// vertices of that face.  Gutter vertices are assigned to
// the closest edge in texture space.
//
// When used with PRT this requires a unique parameterization
// of the model - every texel must correspond to a single point
// on the surface of the model and vice versa

DECLARE_INTERFACE_(ID3DXTextureGutterHelper, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // ID3DXTextureGutterHelper
    
    // dimensions of texture this is bound too
    STDMETHOD_(UINT, GetWidth)(THIS) PURE;
    STDMETHOD_(UINT, GetHeight)(THIS) PURE;


    // Applying gutters recomputes all of the gutter texels of class "2"
    // based on texels of class "1" or "4"
    
    // Applies gutters to a raw float buffer - each texel is NumCoeffs floats
    // Width and Height must match GutterHelper
    STDMETHOD(ApplyGuttersFloat)(THIS_ FLOAT *pDataIn, UINT NumCoeffs, UINT Width, UINT Height);
    
    // Applies gutters to pTexture
    // Dimensions must match GutterHelper
    STDMETHOD(ApplyGuttersTex)(THIS_ LPDIRECT3DTEXTURE9 pTexture);
    
    // Applies gutters to a D3DXPRTBuffer
    // Dimensions must match GutterHelper
    STDMETHOD(ApplyGuttersPRT)(THIS_ LPD3DXPRTBUFFER pBuffer);
       
    // Resamples a texture from a mesh onto this gutterhelpers
    // parameterization.  It is assumed that the UV coordinates
    // for this gutter helper are in TEXTURE 0 (usage/usage index)
    // and the texture coordinates should all be within [0,1] for
    // both sets.
    //
    // pTextureIn - texture represented using parameterization in pMeshIn
    // pMeshIn    - Mesh with texture coordinates that represent pTextureIn
    //              pTextureOut texture coordinates are assumed to be in
    //              TEXTURE 0
    // Usage      - field in DECL for pMeshIn that stores texture coordinates
    //              for pTextureIn
    // UsageIndex - which index for Usage above for pTextureIn
    // pTextureOut- Resampled texture
    // 
    // Usage would generally be D3DDECLUSAGE_TEXCOORD  and UsageIndex other than zero
    STDMETHOD(ResampleTex)(THIS_ LPDIRECT3DTEXTURE9 pTextureIn,
                                 LPD3DXMESH pMeshIn,
                                 D3DDECLUSAGE Usage, UINT UsageIndex,
                                 LPDIRECT3DTEXTURE9 pTextureOut);    
    
    // the routines below provide access to the data structures
    // used by the Apply functions

    // face map is a UINT per texel that represents the
    // face of the mesh that texel belongs too - 
    // only valid if same texel is valid in pGutterData
    // pFaceData must be allocated by the user
    STDMETHOD(GetFaceMap)(THIS_ UINT *pFaceData) PURE;
    
    // BaryMap is a D3DXVECTOR2 per texel
    // the 1st two barycentric coordinates for the corresponding
    // face (3rd weight is always 1-sum of first two)
    // only valid if same texel is valid in pGutterData
    // pBaryData must be allocated by the user
    STDMETHOD(GetBaryMap)(THIS_ D3DXVECTOR2 *pBaryData) PURE;
    
    // TexelMap is a D3DXVECTOR2 per texel that
    // stores the location in pixel coordinates where the
    // corresponding texel is mapped
    // pTexelData must be allocated by the user
    STDMETHOD(GetTexelMap)(THIS_ D3DXVECTOR2 *pTexelData) PURE;
    
    // GutterMap is a BYTE per texel
    // 0/1/2 for Invalid/Internal/Gutter texels
    // 4 represents a gutter texel that will be computed
    // during PRT
    // pGutterData must be allocated by the user
    STDMETHOD(GetGutterMap)(THIS_ BYTE *pGutterData) PURE;
    
    // face map is a UINT per texel that represents the
    // face of the mesh that texel belongs too - 
    // only valid if same texel is valid in pGutterData
    STDMETHOD(SetFaceMap)(THIS_ UINT *pFaceData) PURE;
    
    // BaryMap is a D3DXVECTOR2 per texel
    // the 1st two barycentric coordinates for the corresponding
    // face (3rd weight is always 1-sum of first two)
    // only valid if same texel is valid in pGutterData
    STDMETHOD(SetBaryMap)(THIS_ D3DXVECTOR2 *pBaryData) PURE;
    
    // TexelMap is a D3DXVECTOR2 per texel that
    // stores the location in pixel coordinates where the
    // corresponding texel is mapped
    STDMETHOD(SetTexelMap)(THIS_ D3DXVECTOR2 *pTexelData) PURE;
    
    // GutterMap is a BYTE per texel
    // 0/1/2 for Invalid/Internal/Gutter texels
    // 4 represents a gutter texel that will be computed
    // during PRT
    STDMETHOD(SetGutterMap)(THIS_ BYTE *pGutterData) PURE;    
};


typedef interface ID3DXPRTEngine ID3DXPRTEngine;
typedef interface ID3DXPRTEngine *LPD3DXPRTENGINE;

#undef INTERFACE
#define INTERFACE ID3DXPRTEngine

// ID3DXPRTEngine is used to compute a PRT simulation
// Use the following steps to compute PRT for SH
// (1) create an interface (which includes a scene)
// (2) call SetSamplingInfo
// (3) [optional] Set MeshMaterials/albedo's (required if doing bounces)
// (4) call ComputeDirectLightingSH
// (5) [optional] call ComputeBounce
// repeat step 5 for as many bounces as wanted.
// if you want to model subsurface scattering you
// need to call ComputeSS after direct lighting and
// each bounce.
// If you want to bake the albedo into the PRT signal, you
// must call MutliplyAlbedo, otherwise the user has to multiply
// the albedo themselves.  Not multiplying the albedo allows you
// to model albedo variation at a finer scale then illumination, and
// can result in better compression results.
// Luminance values are computed from RGB values using the following
// formula:  R * 0.2125 + G * 0.7154 + B * 0.0721

DECLARE_INTERFACE_(ID3DXPRTEngine, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // ID3DXPRTEngine
    
    // This sets a material per attribute in the scene mesh and it is
    // the only way to specify subsurface scattering parameters.  if
    // bSetAlbedo is FALSE, NumChannels must match the current
    // configuration of the PRTEngine.  If you intend to change
    // NumChannels (through some other SetAlbedo function) it must
    // happen before SetMeshMaterials is called.
    //
    // NumChannels 1 implies "grayscale" materials, set this to 3 to enable
    //  color bleeding effects
    // bSetAlbedo sets albedo from material if TRUE - which clobbers per texel/vertex
    //  albedo that might have been set before.  FALSE won't clobber.
    // fLengthScale is used for subsurface scattering - scene is mapped into a 1mm unit cube
    //  and scaled by this amount
    STDMETHOD(SetMeshMaterials)(THIS_ CONST D3DXSHMATERIAL **ppMaterials, UINT NumMeshes, 
                                UINT NumChannels, BOOL bSetAlbedo, FLOAT fLengthScale) PURE;
    
    // setting albedo per-vertex or per-texel over rides the albedos stored per mesh
    // but it does not over ride any other settings
    
    // sets an albedo to be used per vertex - the albedo is represented as a float
    // pDataIn input pointer (pointint to albedo of 1st sample)
    // NumChannels 1 implies "grayscale" materials, set this to 3 to enable
    //  color bleeding effects
    // Stride - stride in bytes to get to next samples albedo
    STDMETHOD(SetPerVertexAlbedo)(THIS_ CONST VOID *pDataIn, UINT NumChannels, UINT Stride) PURE;
    
    // represents the albedo per-texel instead of per-vertex (even if per-vertex PRT is used)
    // pAlbedoTexture - texture that stores the albedo (dimension arbitrary)
    // NumChannels 1 implies "grayscale" materials, set this to 3 to enable
    //  color bleeding effects
    // pGH - optional gutter helper, otherwise one is constructed in computation routines and
    //  destroyed (if not attached to buffers)
    STDMETHOD(SetPerTexelAlbedo)(THIS_ LPDIRECT3DTEXTURE9 pAlbedoTexture, 
                                 UINT NumChannels, 
                                 LPD3DXTEXTUREGUTTERHELPER pGH) PURE;
                                 
    // gets the per-vertex albedo
    STDMETHOD(GetVertexAlbedo)(THIS_ D3DXCOLOR *pVertColors, UINT NumVerts) PURE;                                 
                                 
    // If pixel PRT is being computed normals default to ones that are interpolated
    // from the vertex normals.  This specifies a texture that stores an object
    // space normal map instead (must use a texture format that can represent signed values)
    // pNormalTexture - normal map, must be same dimensions as PRTBuffers, signed                                
    STDMETHOD(SetPerTexelNormal)(THIS_ LPDIRECT3DTEXTURE9 pNormalTexture) PURE;
                                 
    // Copies per-vertex albedo from mesh
    // pMesh - mesh that represents the scene.  It must have the same
    //  properties as the mesh used to create the PRTEngine
    // Usage - D3DDECLUSAGE to extract albedos from
    // NumChannels 1 implies "grayscale" materials, set this to 3 to enable
    //  color bleeding effects
    STDMETHOD(ExtractPerVertexAlbedo)(THIS_ LPD3DXMESH pMesh, 
                                      D3DDECLUSAGE Usage, 
                                      UINT NumChannels) PURE;

    // Resamples the input buffer into the output buffer
    // can be used to move between per-vertex and per-texel buffers.  This can also be used
    // to convert single channel buffers to 3-channel buffers and vice-versa.
    STDMETHOD(ResampleBuffer)(THIS_ LPD3DXPRTBUFFER pBufferIn, LPD3DXPRTBUFFER pBufferOut) PURE;
    
    // Returns the scene mesh - including modifications from adaptive spatial sampling
    // The returned mesh only has positions, normals and texture coordinates (if defined)
    // pD3DDevice - d3d device that will be used to allocate the mesh
    // pFaceRemap - each face has a pointer back to the face on the original mesh that it comes from
    //  if the face hasn't been subdivided this will be an identity mapping
    // pVertRemap - each vertex contains 3 vertices that this is a linear combination of
    // pVertWeights - weights for each of above indices (sum to 1.0f)
    // ppMesh - mesh that will be allocated and filled
    STDMETHOD(GetAdaptedMesh)(THIS_ LPDIRECT3DDEVICE9 pD3DDevice,UINT *pFaceRemap, UINT *pVertRemap, FLOAT *pfVertWeights, LPD3DXMESH *ppMesh) PURE;

    // Number of vertices currently allocated (includes new vertices from adaptive sampling)
    STDMETHOD_(UINT, GetNumVerts)(THIS) PURE;
    // Number of faces currently allocated (includes new faces)
    STDMETHOD_(UINT, GetNumFaces)(THIS) PURE;

    // Sets the Minimum/Maximum intersection distances, this can be used to control
    // maximum distance that objects can shadow/reflect light, and help with "bad"
    // art that might have near features that you don't want to shadow.  This does not
    // apply for GPU simulations.
    //  fMin - minimum intersection distance, must be positive and less than fMax
    //  fMax - maximum intersection distance, if 0.0f use the previous value, otherwise
    //      must be strictly greater than fMin
    STDMETHOD(SetMinMaxIntersection)(THIS_ FLOAT fMin, FLOAT fMax) PURE;

    // This will subdivide faces on a mesh so that adaptively simulations can
    // use a more conservative threshold (it won't miss features.)
    // MinEdgeLength - minimum edge length that will be generated, if 0.0f a
    //  reasonable default will be used
    // MaxSubdiv - maximum level of subdivision, if 0 is specified a default
    //  value will be used (5)
    STDMETHOD(RobustMeshRefine)(THIS_ FLOAT MinEdgeLength, UINT MaxSubdiv) PURE;

    // This sets to sampling information used by the simulator.  Adaptive sampling
    // parameters are currently ignored.
    // NumRays - number of rays to shoot per sample
    // UseSphere - if TRUE uses spherical samples, otherwise samples over
    //  the hemisphere.  Should only be used with GPU and Vol computations
    // UseCosine - if TRUE uses a cosine weighting - not used for Vol computations
    //  or if only the visiblity function is desired
    // Adaptive - if TRUE adaptive sampling (angular) is used
    // AdaptiveThresh - threshold used to terminate adaptive angular sampling
    //  ignored if adaptive sampling is not set
    STDMETHOD(SetSamplingInfo)(THIS_ UINT NumRays, 
                               BOOL UseSphere, 
                               BOOL UseCosine, 
                               BOOL Adaptive, 
                               FLOAT AdaptiveThresh) PURE;

    // Methods that compute the direct lighting contribution for objects
    // always represente light using spherical harmonics (SH)
    // the albedo is not multiplied by the signal - it just integrates
    // incoming light.  If NumChannels is not 1 the vector is replicated
    //
    // SHOrder - order of SH to use
    // pDataOut - PRT buffer that is generated.  Can be single channel
    STDMETHOD(ComputeDirectLightingSH)(THIS_ UINT SHOrder, 
                                       LPD3DXPRTBUFFER pDataOut) PURE;
                                       
    // Adaptive variant of above function.  This will refine the mesh
    // generating new vertices/faces to approximate the PRT signal
    // more faithfully.
    // SHOrder - order of SH to use
    // AdaptiveThresh - threshold for adaptive subdivision (in PRT vector error)
    //  if value is less then 1e-6f, 1e-6f is specified
    // MinEdgeLength - minimum edge length that will be generated
    //  if value is too small a fairly conservative model dependent value
    //  is used
    // MaxSubdiv - maximum subdivision level, if 0 is specified it 
    //  will default to 4
    // pDataOut - PRT buffer that is generated.  Can be single channel.
    STDMETHOD(ComputeDirectLightingSHAdaptive)(THIS_ UINT SHOrder, 
                                               FLOAT AdaptiveThresh,
                                               FLOAT MinEdgeLength,
                                               UINT MaxSubdiv,
                                               LPD3DXPRTBUFFER pDataOut) PURE;
                                       
    // Function that computes the direct lighting contribution for objects
    // light is always represented using spherical harmonics (SH)
    // This is done on the GPU and is much faster then using the CPU.
    // The albedo is not multiplied by the signal - it just integrates
    // incoming light.  If NumChannels is not 1 the vector is replicated.
    // ZBias/ZAngleBias are akin to parameters used with shadow zbuffers.
    // A reasonable default for both values is 0.005, but the user should
    // experiment (ZAngleBias can be zero, ZBias should not be.)
    // Callbacks should not use the Direct3D9Device the simulator is using.
    // SetSamplingInfo must be called with TRUE for UseSphere and
    // FALSE for UseCosine before this method is called.
    //
    // pD3DDevice - device used to run GPU simulator - must support PS2.0
    //  and FP render targets
    // Flags - parameters for the GPU simulator, combination of one or more
    //  D3DXSHGPUSIMOPT flags.  Only one SHADOWRES setting should be set and
    //  the defaults is 512
    // SHOrder - order of SH to use
    // ZBias - bias in normal direction (for depth test)
    // ZAngleBias - scaled by one minus cosine of angle with light (offset in depth)
    // pDataOut - PRT buffer that is filled in.  Can be single channel
    STDMETHOD(ComputeDirectLightingSHGPU)(THIS_ LPDIRECT3DDEVICE9 pD3DDevice,
                                          UINT Flags,
                                          UINT SHOrder,
                                          FLOAT ZBias,
                                          FLOAT ZAngleBias,
                                          LPD3DXPRTBUFFER pDataOut) PURE;


    // Functions that computes subsurface scattering (using material properties)
    // Albedo is not multiplied by result.  This only works for per-vertex data
    // use ResampleBuffer to move per-vertex data into a texture and back.
    //
    // pDataIn - input data (previous bounce)
    // pDataOut - result of subsurface scattering simulation
    // pDataTotal - [optional] results can be summed into this buffer
    STDMETHOD(ComputeSS)(THIS_ LPD3DXPRTBUFFER pDataIn, 
                         LPD3DXPRTBUFFER pDataOut, LPD3DXPRTBUFFER pDataTotal) PURE;

    // Adaptive version of ComputeSS.
    //
    // pDataIn - input data (previous bounce)
    // AdaptiveThresh - threshold for adaptive subdivision (in PRT vector error)
    //  if value is less then 1e-6f, 1e-6f is specified
    // MinEdgeLength - minimum edge length that will be generated
    //  if value is too small a fairly conservative model dependent value
    //  is used
    // MaxSubdiv - maximum subdivision level, if 0 is specified it 
    //  will default to 4    
    // pDataOut - result of subsurface scattering simulation
    // pDataTotal - [optional] results can be summed into this buffer
    STDMETHOD(ComputeSSAdaptive)(THIS_ LPD3DXPRTBUFFER pDataIn, 
                                 FLOAT AdaptiveThresh,
                                 FLOAT MinEdgeLength,
                                 UINT MaxSubdiv,
                                 LPD3DXPRTBUFFER pDataOut, LPD3DXPRTBUFFER pDataTotal) PURE;

    // computes a single bounce of inter-reflected light
    // works for SH based PRT or generic lighting
    // Albedo is not multiplied by result
    //
    // pDataIn - previous bounces data 
    // pDataOut - PRT buffer that is generated
    // pDataTotal - [optional] can be used to keep a running sum
    STDMETHOD(ComputeBounce)(THIS_ LPD3DXPRTBUFFER pDataIn,
                             LPD3DXPRTBUFFER pDataOut,
                             LPD3DXPRTBUFFER pDataTotal) PURE;

    // Adaptive version of above function.
    //
    // pDataIn - previous bounces data, can be single channel 
    // AdaptiveThresh - threshold for adaptive subdivision (in PRT vector error)
    //  if value is less then 1e-6f, 1e-6f is specified
    // MinEdgeLength - minimum edge length that will be generated
    //  if value is too small a fairly conservative model dependent value
    //  is used
    // MaxSubdiv - maximum subdivision level, if 0 is specified it 
    //  will default to 4
    // pDataOut - PRT buffer that is generated
    // pDataTotal - [optional] can be used to keep a running sum    
    STDMETHOD(ComputeBounceAdaptive)(THIS_ LPD3DXPRTBUFFER pDataIn,
                                     FLOAT AdaptiveThresh,
                                     FLOAT MinEdgeLength,
                                     UINT MaxSubdiv,
                                     LPD3DXPRTBUFFER pDataOut,
                                     LPD3DXPRTBUFFER pDataTotal) PURE;

    // Computes projection of distant SH radiance into a local SH radiance
    // function.  This models how direct lighting is attenuated by the 
    // scene and is a form of "neighborhood transfer."  The result is
    // a linear operator (matrix) at every sample point, if you multiply
    // this matrix by the distant SH lighting coefficients you get an
    // approximation of the local incident radiance function from
    // direct lighting.  These resulting lighting coefficients can
    // than be projected into another basis or used with any rendering
    // technique that uses spherical harmonics as input.
    // SetSamplingInfo must be called with TRUE for UseSphere and
    // FALSE for UseCosine before this method is called.  
    // Generates SHOrderIn*SHOrderIn*SHOrderOut*SHOrderOut scalars 
    // per channel at each sample location.
    //
    // SHOrderIn  - Order of the SH representation of distant lighting
    // SHOrderOut - Order of the SH representation of local lighting
    // NumVolSamples  - Number of sample locations
    // pSampleLocs    - position of sample locations
    // pDataOut       - PRT Buffer that will store output results    
    STDMETHOD(ComputeVolumeSamplesDirectSH)(THIS_ UINT SHOrderIn, 
                                            UINT SHOrderOut, 
                                            UINT NumVolSamples,
                                            CONST D3DXVECTOR3 *pSampleLocs,
                                            LPD3DXPRTBUFFER pDataOut) PURE;
                                    
    // At each sample location computes a linear operator (matrix) that maps
    // the representation of source radiance (NumCoeffs in pSurfDataIn)
    // into a local incident radiance function approximated with spherical 
    // harmonics.  For example if a light map data is specified in pSurfDataIn
    // the result is an SH representation of the flow of light at each sample
    // point.  If PRT data for an outdoor scene is used, each sample point
    // contains a matrix that models how distant lighting bounces of the objects
    // in the scene and arrives at the given sample point.  Combined with
    // ComputeVolumeSamplesDirectSH this gives the complete representation for
    // how light arrives at each sample point parameterized by distant lighting.
    // SetSamplingInfo must be called with TRUE for UseSphere and
    // FALSE for UseCosine before this method is called.    
    // Generates pSurfDataIn->NumCoeffs()*SHOrder*SHOrder scalars
    // per channel at each sample location.
    //
    // pSurfDataIn    - previous bounce data
    // SHOrder        - order of SH to generate projection with
    // NumVolSamples  - Number of sample locations
    // pSampleLocs    - position of sample locations
    // pDataOut       - PRT Buffer that will store output results
    STDMETHOD(ComputeVolumeSamples)(THIS_ LPD3DXPRTBUFFER pSurfDataIn, 
                                    UINT SHOrder, 
                                    UINT NumVolSamples,
                                    CONST D3DXVECTOR3 *pSampleLocs,
                                    LPD3DXPRTBUFFER pDataOut) PURE;

    // Computes direct lighting (SH) for a point not on the mesh
    // with a given normal - cannot use texture buffers.
    //
    // SHOrder      - order of SH to use
    // NumSamples   - number of sample locations
    // pSampleLocs  - position for each sample
    // pSampleNorms - normal for each sample
    // pDataOut     - PRT Buffer that will store output results
    STDMETHOD(ComputeSurfSamplesDirectSH)(THIS_ UINT SHOrder,
                                          UINT NumSamples,
                                          CONST D3DXVECTOR3 *pSampleLocs,
                                          CONST D3DXVECTOR3 *pSampleNorms,
                                          LPD3DXPRTBUFFER pDataOut) PURE;


    // given the solution for PRT or light maps, computes transfer vector at arbitrary 
    // position/normal pairs in space
    //
    // pSurfDataIn  - input data
    // NumSamples   - number of sample locations
    // pSampleLocs  - position for each sample
    // pSampleNorms - normal for each sample
    // pDataOut     - PRT Buffer that will store output results
    // pDataTotal   - optional buffer to sum results into - can be NULL
    STDMETHOD(ComputeSurfSamplesBounce)(THIS_ LPD3DXPRTBUFFER pSurfDataIn,
                                        UINT NumSamples,
                                        CONST D3DXVECTOR3 *pSampleLocs,
                                        CONST D3DXVECTOR3 *pSampleNorms,
                                        LPD3DXPRTBUFFER pDataOut,
                                        LPD3DXPRTBUFFER pDataTotal) PURE;

    // Frees temporary data structures that can be created for subsurface scattering
    // this data is freed when the PRTComputeEngine is freed and is lazily created
    STDMETHOD(FreeSSData)(THIS) PURE;
    
    // Frees temporary data structures that can be created for bounce simulations
    // this data is freed when the PRTComputeEngine is freed and is lazily created
    STDMETHOD(FreeBounceData)(THIS) PURE;

    // This computes the Local Deformable PRT (LDPRT) coefficients relative to the 
    // per sample normals that minimize error in a least squares sense with respect 
    // to the input PRT data set.  These coefficients can be used with skinned/transformed 
    // normals to model global effects with dynamic objects.  Shading normals can 
    // optionally be solved for - these normals (along with the LDPRT coefficients) can
    // more accurately represent the PRT signal.  The coefficients are for zonal
    // harmonics oriented in the normal/shading normal direction.
    //
    // pDataIn  - SH PRT dataset that is input
    // SHOrder  - Order of SH to compute conv coefficients for 
    // pNormOut - Optional array of vectors (passed in) that will be filled with
    //             "shading normals", LDPRT coefficients are optimized for
    //             these normals.  This array must be the same size as the number of
    //             samples in pDataIn
    // pDataOut - Output buffer (SHOrder zonal harmonic coefficients per channel per sample)
    STDMETHOD(ComputeLDPRTCoeffs)(THIS_ LPD3DXPRTBUFFER pDataIn,
                                  UINT SHOrder,
                                  D3DXVECTOR3 *pNormOut,
                                  LPD3DXPRTBUFFER pDataOut) PURE;

    // scales all the samples associated with a given sub mesh
    // can be useful when using subsurface scattering
    // fScale - value to scale each vector in submesh by
    STDMETHOD(ScaleMeshChunk)(THIS_ UINT uMeshChunk, FLOAT fScale, LPD3DXPRTBUFFER pDataOut) PURE;
    
    // mutliplies each PRT vector by the albedo - can be used if you want to have the albedo
    // burned into the dataset, often better not to do this.  If this is not done the user
    // must mutliply the albedo themselves when rendering - just multiply the albedo times
    // the result of the PRT dot product.
    // If pDataOut is a texture simulation result and there is an albedo texture it
    // must be represented at the same resolution as the simulation buffer.  You can use
    // LoadSurfaceFromSurface and set a new albedo texture if this is an issue - but must
    // be careful about how the gutters are handled.
    //
    // pDataOut - dataset that will get albedo pushed into it
    STDMETHOD(MultiplyAlbedo)(THIS_ LPD3DXPRTBUFFER pDataOut) PURE;
    
    // Sets a pointer to an optional call back function that reports back to the
    // user percentage done and gives them the option of quitting
    // pCB - pointer to call back function, return S_OK for the simulation
    //  to continue
    // Frequency - 1/Frequency is roughly the number of times the call back
    //  will be invoked
    // lpUserContext - will be passed back to the users call back
    STDMETHOD(SetCallBack)(THIS_ LPD3DXSHPRTSIMCB pCB, FLOAT Frequency,  LPVOID lpUserContext) PURE;
    
    // Returns TRUE if the ray intersects the mesh, FALSE if it does not.  This function
    // takes into account settings from SetMinMaxIntersection.  If the closest intersection
    // is not needed this function is more efficient compared to the ClosestRayIntersection
    // method.
    // pRayPos - origin of ray
    // pRayDir - normalized ray direction (normalization required for SetMinMax to be meaningful)
    
    STDMETHOD_(BOOL, ShadowRayIntersects)(THIS_ CONST D3DXVECTOR3 *pRayPos, CONST D3DXVECTOR3 *pRayDir) PURE;
    
    // Returns TRUE if the ray intersects the mesh, FALSE if it does not.  If there is an
    // intersection the closest face that was intersected and its first two barycentric coordinates
    // are returned.  This function takes into account settings from SetMinMaxIntersection.
    // This is a slower function compared to ShadowRayIntersects and should only be used where
    // needed.  The third vertices barycentric coordinates will be 1 - pU - pV.
    // pRayPos     - origin of ray
    // pRayDir     - normalized ray direction (normalization required for SetMinMax to be meaningful)
    // pFaceIndex  - Closest face that intersects.  This index is based on stacking the pBlockerMesh
    //  faces before the faces from pMesh
    // pU          - Barycentric coordinate for vertex 0
    // pV          - Barycentric coordinate for vertex 1
    // pDist       - Distance along ray where the intersection occured
    
    STDMETHOD_(BOOL, ClosestRayIntersects)(THIS_ CONST D3DXVECTOR3 *pRayPos, CONST D3DXVECTOR3 *pRayDir,
                                           DWORD *pFaceIndex, FLOAT *pU, FLOAT *pV, FLOAT *pDist) PURE;
};


// API functions for creating interfaces

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

//============================================================================
//
//  D3DXCreatePRTBuffer:
//  --------------------
//  Generates a PRT Buffer that can be compressed or filled by a simulator
//  This function should be used to create per-vertex or volume buffers.
//  When buffers are created all values are initialized to zero.
//
//  Parameters:
//    NumSamples
//      Number of sample locations represented
//    NumCoeffs
//      Number of coefficients per sample location (order^2 for SH)
//    NumChannels
//      Number of color channels to represent (1 or 3)
//    ppBuffer
//      Buffer that will be allocated
//
//============================================================================

HRESULT WINAPI 
    D3DXCreatePRTBuffer( 
        UINT NumSamples,
        UINT NumCoeffs,
        UINT NumChannels,
        LPD3DXPRTBUFFER* ppBuffer);

//============================================================================
//
//  D3DXCreatePRTBufferTex:
//  --------------------
//  Generates a PRT Buffer that can be compressed or filled by a simulator
//  This function should be used to create per-pixel buffers.
//  When buffers are created all values are initialized to zero.
//
//  Parameters:
//    Width
//      Width of texture
//    Height
//      Height of texture
//    NumCoeffs
//      Number of coefficients per sample location (order^2 for SH)
//    NumChannels
//      Number of color channels to represent (1 or 3)
//    ppBuffer
//      Buffer that will be allocated
//
//============================================================================

HRESULT WINAPI
    D3DXCreatePRTBufferTex( 
        UINT Width,
        UINT Height,
        UINT NumCoeffs,
        UINT NumChannels,
        LPD3DXPRTBUFFER* ppBuffer);

//============================================================================
//
//  D3DXLoadPRTBufferFromFile:
//  --------------------
//  Loads a PRT buffer that has been saved to disk.
//
//  Parameters:
//    pFilename
//      Name of the file to load
//    ppBuffer
//      Buffer that will be allocated
//
//============================================================================

HRESULT WINAPI
    D3DXLoadPRTBufferFromFileA(
        LPCSTR pFilename, 
        LPD3DXPRTBUFFER*       ppBuffer);
        
HRESULT WINAPI
    D3DXLoadPRTBufferFromFileW(
        LPCWSTR pFilename, 
        LPD3DXPRTBUFFER*       ppBuffer);

#ifdef UNICODE
#define D3DXLoadPRTBufferFromFile D3DXLoadPRTBufferFromFileW
#else
#define D3DXLoadPRTBufferFromFile D3DXLoadPRTBufferFromFileA
#endif


//============================================================================
//
//  D3DXSavePRTBufferToFile:
//  --------------------
//  Saves a PRTBuffer to disk.
//
//  Parameters:
//    pFilename
//      Name of the file to save
//    pBuffer
//      Buffer that will be saved
//
//============================================================================

HRESULT WINAPI
    D3DXSavePRTBufferToFileA(
        LPCSTR pFileName,
        LPD3DXPRTBUFFER pBuffer);
        
HRESULT WINAPI
    D3DXSavePRTBufferToFileW(
        LPCWSTR pFileName,
        LPD3DXPRTBUFFER pBuffer);

#ifdef UNICODE
#define D3DXSavePRTBufferToFile D3DXSavePRTBufferToFileW
#else
#define D3DXSavePRTBufferToFile D3DXSavePRTBufferToFileA
#endif                


//============================================================================
//
//  D3DXLoadPRTCompBufferFromFile:
//  --------------------
//  Loads a PRTComp buffer that has been saved to disk.
//
//  Parameters:
//    pFilename
//      Name of the file to load
//    ppBuffer
//      Buffer that will be allocated
//
//============================================================================

HRESULT WINAPI
    D3DXLoadPRTCompBufferFromFileA(
        LPCSTR pFilename, 
        LPD3DXPRTCOMPBUFFER*       ppBuffer);
        
HRESULT WINAPI
    D3DXLoadPRTCompBufferFromFileW(
        LPCWSTR pFilename, 
        LPD3DXPRTCOMPBUFFER*       ppBuffer);

#ifdef UNICODE
#define D3DXLoadPRTCompBufferFromFile D3DXLoadPRTCompBufferFromFileW
#else
#define D3DXLoadPRTCompBufferFromFile D3DXLoadPRTCompBufferFromFileA
#endif

//============================================================================
//
//  D3DXSavePRTCompBufferToFile:
//  --------------------
//  Saves a PRTCompBuffer to disk.
//
//  Parameters:
//    pFilename
//      Name of the file to save
//    pBuffer
//      Buffer that will be saved
//
//============================================================================

HRESULT WINAPI
    D3DXSavePRTCompBufferToFileA(
        LPCSTR pFileName,
        LPD3DXPRTCOMPBUFFER pBuffer);
        
HRESULT WINAPI
    D3DXSavePRTCompBufferToFileW(
        LPCWSTR pFileName,
        LPD3DXPRTCOMPBUFFER pBuffer);

#ifdef UNICODE
#define D3DXSavePRTCompBufferToFile D3DXSavePRTCompBufferToFileW
#else
#define D3DXSavePRTCompBufferToFile D3DXSavePRTCompBufferToFileA
#endif 

//============================================================================
//
//  D3DXCreatePRTCompBuffer:
//  --------------------
//  Compresses a PRT buffer (vertex or texel)
//
//  Parameters:
//    D3DXSHCOMPRESSQUALITYTYPE
//      Quality of compression - low is faster (computes PCA per voronoi cluster)
//      high is slower but better quality (clusters based on distance to affine subspace)
//    NumClusters
//      Number of clusters to compute
//    NumPCA
//      Number of basis vectors to compute
//    pCB
//      Optional Callback function
//    lpUserContext
//      Optional user context
//    pBufferIn
//      Buffer that will be compressed
//    ppBufferOut
//      Compressed buffer that will be created
//
//============================================================================


HRESULT WINAPI
    D3DXCreatePRTCompBuffer(
        D3DXSHCOMPRESSQUALITYTYPE Quality,
        UINT NumClusters, 
        UINT NumPCA,
        LPD3DXSHPRTSIMCB pCB,
        LPVOID lpUserContext,        
        LPD3DXPRTBUFFER  pBufferIn,
        LPD3DXPRTCOMPBUFFER *ppBufferOut
    );

//============================================================================
//
//  D3DXCreateTextureGutterHelper:
//  --------------------
//  Generates a "GutterHelper" for a given set of meshes and texture
//  resolution
//
//  Parameters:
//    Width
//      Width of texture
//    Height
//      Height of texture
//    pMesh
//      Mesh that represents the scene
//    GutterSize
//      Number of texels to over rasterize in texture space
//      this should be at least 1.0
//    ppBuffer
//      GutterHelper that will be created
//
//============================================================================


HRESULT WINAPI 
    D3DXCreateTextureGutterHelper( 
        UINT Width,
        UINT Height,
        LPD3DXMESH pMesh, 
        FLOAT GutterSize,
        LPD3DXTEXTUREGUTTERHELPER* ppBuffer);


//============================================================================
//
//  D3DXCreatePRTEngine:
//  --------------------
//  Computes a PRTEngine which can efficiently generate PRT simulations
//  of a scene
//
//  Parameters:
//    pMesh
//      Mesh that represents the scene - must have an AttributeTable
//      where vertices are in a unique attribute.
//    pAdjacency
//      Optional adjacency information
//    ExtractUVs
//      Set this to true if textures are going to be used for albedos
//      or to store PRT vectors
//    pBlockerMesh
//      Optional mesh that just blocks the scene
//    ppEngine
//      PRTEngine that will be created
//
//============================================================================


HRESULT WINAPI 
    D3DXCreatePRTEngine( 
        LPD3DXMESH pMesh,
        DWORD *pAdjacency,
        BOOL ExtractUVs,
        LPD3DXMESH pBlockerMesh, 
        LPD3DXPRTENGINE* ppEngine);

//============================================================================
//
//  D3DXConcatenateMeshes:
//  --------------------
//  Concatenates a group of meshes into one common mesh.  This can optionally transform
//  each sub mesh or its texture coordinates.  If no DECL is given it will
//  generate a union of all of the DECL's of the sub meshes, promoting channels
//  and types if necessary.  It will create an AttributeTable if possible, one can
//  call OptimizeMesh with attribute sort and compacting enabled to ensure this.
//
//  Parameters:
//    ppMeshes
//      Array of pointers to meshes that can store PRT vectors
//    NumMeshes
//      Number of meshes
//    Options
//      Passed through to D3DXCreateMesh
//    pGeomXForms
//      [optional] Each sub mesh is transformed by the corresponding
//      matrix if this array is supplied
//    pTextureXForms
//      [optional] UV coordinates for each sub mesh are transformed
//      by corresponding matrix if supplied
//    pDecl
//      [optional] Only information in this DECL is used when merging
//      data
//    pD3DDevice
//      D3D device that is used to create the new mesh
//    ppMeshOut
//      Mesh that will be created
//
//============================================================================


HRESULT WINAPI 
    D3DXConcatenateMeshes(
        LPD3DXMESH *ppMeshes, 
        UINT NumMeshes, 
        DWORD Options, 
        CONST D3DXMATRIX *pGeomXForms, 
        CONST D3DXMATRIX *pTextureXForms, 
        CONST D3DVERTEXELEMENT9 *pDecl,
        LPDIRECT3DDEVICE9 pD3DDevice, 
        LPD3DXMESH *ppMeshOut);

//============================================================================
//
//  D3DXSHPRTCompSuperCluster:
//  --------------------------
//  Used with compressed results of D3DXSHPRTSimulation.
//  Generates "super clusters" - groups of clusters that can be drawn in
//  the same draw call.  A greedy algorithm that minimizes overdraw is used
//  to group the clusters.
//
//  Parameters:
//   pClusterIDs
//      NumVerts cluster ID's (extracted from a compressed buffer)
//   pScene
//      Mesh that represents composite scene passed to the simulator
//   MaxNumClusters
//      Maximum number of clusters allocated per super cluster
//   NumClusters
//      Number of clusters computed in the simulator
//   pSuperClusterIDs
//      Array of length NumClusters, contains index of super cluster
//      that corresponding cluster was assigned to
//   pNumSuperClusters
//      Returns the number of super clusters allocated
//      
//============================================================================

HRESULT WINAPI 
    D3DXSHPRTCompSuperCluster(
        UINT *pClusterIDs, 
        LPD3DXMESH pScene, 
        UINT MaxNumClusters, 
        UINT NumClusters,
        UINT *pSuperClusterIDs, 
        UINT *pNumSuperClusters);

//============================================================================
//
//  D3DXSHPRTCompSplitMeshSC:
//  -------------------------
//  Used with compressed results of the vertex version of the PRT simulator.
//  After D3DXSHRTCompSuperCluster has been called this function can be used
//  to split the mesh into a group of faces/vertices per super cluster.
//  Each super cluster contains all of the faces that contain any vertex
//  classified in one of its clusters.  All of the vertices connected to this
//  set of faces are also included with the returned array ppVertStatus 
//  indicating whether or not the vertex belongs to the supercluster.
//
//  Parameters:
//   pClusterIDs
//      NumVerts cluster ID's (extracted from a compressed buffer)
//   NumVertices
//      Number of vertices in original mesh
//   NumClusters
//      Number of clusters (input parameter to compression)
//   pSuperClusterIDs
//      Array of size NumClusters that will contain super cluster ID's (from
//      D3DXSHCompSuerCluster)
//   NumSuperClusters
//      Number of superclusters allocated in D3DXSHCompSuerCluster
//   pInputIB
//      Raw index buffer for mesh - format depends on bInputIBIs32Bit
//   InputIBIs32Bit
//      Indicates whether the input index buffer is 32-bit (otherwise 16-bit
//      is assumed)
//   NumFaces
//      Number of faces in the original mesh (pInputIB is 3 times this length)
//   ppIBData
//      LPD3DXBUFFER holds raw index buffer that will contain the resulting split faces.  
//      Format determined by bIBIs32Bit.  Allocated by function
//   pIBDataLength
//      Length of ppIBData, assigned in function
//   OutputIBIs32Bit
//      Indicates whether the output index buffer is to be 32-bit (otherwise 
//      16-bit is assumed)
//   ppFaceRemap
//      LPD3DXBUFFER mapping of each face in ppIBData to original faces.  Length is
//      *pIBDataLength/3.  Optional paramter, allocated in function
//   ppVertData
//      LPD3DXBUFFER contains new vertex data structure.  Size of pVertDataLength
//   pVertDataLength
//      Number of new vertices in split mesh.  Assigned in function
//   pSCClusterList
//      Array of length NumClusters which pSCData indexes into (Cluster* fields)
//      for each SC, contains clusters sorted by super cluster
//   pSCData
//      Structure per super cluster - contains indices into ppIBData,
//      pSCClusterList and ppVertData
//
//============================================================================

HRESULT WINAPI 
    D3DXSHPRTCompSplitMeshSC(
        UINT *pClusterIDs, 
        UINT NumVertices, 
        UINT NumClusters, 
        UINT *pSuperClusterIDs, 
        UINT NumSuperClusters,
        LPVOID pInputIB, 
        BOOL InputIBIs32Bit, 
        UINT NumFaces,
        LPD3DXBUFFER *ppIBData, 
        UINT *pIBDataLength, 
        BOOL OutputIBIs32Bit, 
        LPD3DXBUFFER *ppFaceRemap, 
        LPD3DXBUFFER *ppVertData, 
        UINT *pVertDataLength, 
        UINT *pSCClusterList,
        D3DXSHPRTSPLITMESHCLUSTERDATA *pSCData);
        
        
#ifdef __cplusplus
}
#endif //__cplusplus

//////////////////////////////////////////////////////////////////////////////
//
//  Definitions of .X file templates used by mesh load/save functions 
//    that are not RM standard
//
//////////////////////////////////////////////////////////////////////////////

// {3CF169CE-FF7C-44ab-93C0-F78F62D172E2}
DEFINE_GUID(DXFILEOBJ_XSkinMeshHeader,
0x3cf169ce, 0xff7c, 0x44ab, 0x93, 0xc0, 0xf7, 0x8f, 0x62, 0xd1, 0x72, 0xe2);

// {B8D65549-D7C9-4995-89CF-53A9A8B031E3}
DEFINE_GUID(DXFILEOBJ_VertexDuplicationIndices, 
0xb8d65549, 0xd7c9, 0x4995, 0x89, 0xcf, 0x53, 0xa9, 0xa8, 0xb0, 0x31, 0xe3);

// {A64C844A-E282-4756-8B80-250CDE04398C}
DEFINE_GUID(DXFILEOBJ_FaceAdjacency, 
0xa64c844a, 0xe282, 0x4756, 0x8b, 0x80, 0x25, 0xc, 0xde, 0x4, 0x39, 0x8c);

// {6F0D123B-BAD2-4167-A0D0-80224F25FABB}
DEFINE_GUID(DXFILEOBJ_SkinWeights, 
0x6f0d123b, 0xbad2, 0x4167, 0xa0, 0xd0, 0x80, 0x22, 0x4f, 0x25, 0xfa, 0xbb);

// {A3EB5D44-FC22-429d-9AFB-3221CB9719A6}
DEFINE_GUID(DXFILEOBJ_Patch, 
0xa3eb5d44, 0xfc22, 0x429d, 0x9a, 0xfb, 0x32, 0x21, 0xcb, 0x97, 0x19, 0xa6);

// {D02C95CC-EDBA-4305-9B5D-1820D7704BBF}
DEFINE_GUID(DXFILEOBJ_PatchMesh, 
0xd02c95cc, 0xedba, 0x4305, 0x9b, 0x5d, 0x18, 0x20, 0xd7, 0x70, 0x4b, 0xbf);

// {B9EC94E1-B9A6-4251-BA18-94893F02C0EA}
DEFINE_GUID(DXFILEOBJ_PatchMesh9, 
0xb9ec94e1, 0xb9a6, 0x4251, 0xba, 0x18, 0x94, 0x89, 0x3f, 0x2, 0xc0, 0xea);

// {B6C3E656-EC8B-4b92-9B62-681659522947}
DEFINE_GUID(DXFILEOBJ_PMInfo, 
0xb6c3e656, 0xec8b, 0x4b92, 0x9b, 0x62, 0x68, 0x16, 0x59, 0x52, 0x29, 0x47);

// {917E0427-C61E-4a14-9C64-AFE65F9E9844}
DEFINE_GUID(DXFILEOBJ_PMAttributeRange, 
0x917e0427, 0xc61e, 0x4a14, 0x9c, 0x64, 0xaf, 0xe6, 0x5f, 0x9e, 0x98, 0x44);

// {574CCC14-F0B3-4333-822D-93E8A8A08E4C}
DEFINE_GUID(DXFILEOBJ_PMVSplitRecord,
0x574ccc14, 0xf0b3, 0x4333, 0x82, 0x2d, 0x93, 0xe8, 0xa8, 0xa0, 0x8e, 0x4c);

// {B6E70A0E-8EF9-4e83-94AD-ECC8B0C04897}
DEFINE_GUID(DXFILEOBJ_FVFData, 
0xb6e70a0e, 0x8ef9, 0x4e83, 0x94, 0xad, 0xec, 0xc8, 0xb0, 0xc0, 0x48, 0x97);

// {F752461C-1E23-48f6-B9F8-8350850F336F}
DEFINE_GUID(DXFILEOBJ_VertexElement, 
0xf752461c, 0x1e23, 0x48f6, 0xb9, 0xf8, 0x83, 0x50, 0x85, 0xf, 0x33, 0x6f);

// {BF22E553-292C-4781-9FEA-62BD554BDD93}
DEFINE_GUID(DXFILEOBJ_DeclData, 
0xbf22e553, 0x292c, 0x4781, 0x9f, 0xea, 0x62, 0xbd, 0x55, 0x4b, 0xdd, 0x93);

// {F1CFE2B3-0DE3-4e28-AFA1-155A750A282D}
DEFINE_GUID(DXFILEOBJ_EffectFloats, 
0xf1cfe2b3, 0xde3, 0x4e28, 0xaf, 0xa1, 0x15, 0x5a, 0x75, 0xa, 0x28, 0x2d);

// {D55B097E-BDB6-4c52-B03D-6051C89D0E42}
DEFINE_GUID(DXFILEOBJ_EffectString, 
0xd55b097e, 0xbdb6, 0x4c52, 0xb0, 0x3d, 0x60, 0x51, 0xc8, 0x9d, 0xe, 0x42);

// {622C0ED0-956E-4da9-908A-2AF94F3CE716}
DEFINE_GUID(DXFILEOBJ_EffectDWord, 
0x622c0ed0, 0x956e, 0x4da9, 0x90, 0x8a, 0x2a, 0xf9, 0x4f, 0x3c, 0xe7, 0x16);

// {3014B9A0-62F5-478c-9B86-E4AC9F4E418B}
DEFINE_GUID(DXFILEOBJ_EffectParamFloats, 
0x3014b9a0, 0x62f5, 0x478c, 0x9b, 0x86, 0xe4, 0xac, 0x9f, 0x4e, 0x41, 0x8b);

// {1DBC4C88-94C1-46ee-9076-2C28818C9481}
DEFINE_GUID(DXFILEOBJ_EffectParamString, 
0x1dbc4c88, 0x94c1, 0x46ee, 0x90, 0x76, 0x2c, 0x28, 0x81, 0x8c, 0x94, 0x81);

// {E13963BC-AE51-4c5d-B00F-CFA3A9D97CE5}
DEFINE_GUID(DXFILEOBJ_EffectParamDWord,
0xe13963bc, 0xae51, 0x4c5d, 0xb0, 0xf, 0xcf, 0xa3, 0xa9, 0xd9, 0x7c, 0xe5);

// {E331F7E4-0559-4cc2-8E99-1CEC1657928F}
DEFINE_GUID(DXFILEOBJ_EffectInstance, 
0xe331f7e4, 0x559, 0x4cc2, 0x8e, 0x99, 0x1c, 0xec, 0x16, 0x57, 0x92, 0x8f);

// {9E415A43-7BA6-4a73-8743-B73D47E88476}
DEFINE_GUID(DXFILEOBJ_AnimTicksPerSecond, 
0x9e415a43, 0x7ba6, 0x4a73, 0x87, 0x43, 0xb7, 0x3d, 0x47, 0xe8, 0x84, 0x76);

// {7F9B00B3-F125-4890-876E-1CFFBF697C4D}
DEFINE_GUID(DXFILEOBJ_CompressedAnimationSet, 
0x7f9b00b3, 0xf125, 0x4890, 0x87, 0x6e, 0x1c, 0x42, 0xbf, 0x69, 0x7c, 0x4d);

#pragma pack(push, 1)
typedef struct _XFILECOMPRESSEDANIMATIONSET
{
    DWORD CompressedBlockSize;
    FLOAT TicksPerSec;
    DWORD PlaybackType;
    DWORD BufferLength;
} XFILECOMPRESSEDANIMATIONSET;
#pragma pack(pop)

#define XSKINEXP_TEMPLATES \
        "xof 0303txt 0032\
        template XSkinMeshHeader \
        { \
            <3CF169CE-FF7C-44ab-93C0-F78F62D172E2> \
            WORD nMaxSkinWeightsPerVertex; \
            WORD nMaxSkinWeightsPerFace; \
            WORD nBones; \
        } \
        template VertexDuplicationIndices \
        { \
            <B8D65549-D7C9-4995-89CF-53A9A8B031E3> \
            DWORD nIndices; \
            DWORD nOriginalVertices; \
            array DWORD indices[nIndices]; \
        } \
        template FaceAdjacency \
        { \
            <A64C844A-E282-4756-8B80-250CDE04398C> \
            DWORD nIndices; \
            array DWORD indices[nIndices]; \
        } \
        template SkinWeights \
        { \
            <6F0D123B-BAD2-4167-A0D0-80224F25FABB> \
            STRING transformNodeName; \
            DWORD nWeights; \
            array DWORD vertexIndices[nWeights]; \
            array float weights[nWeights]; \
            Matrix4x4 matrixOffset; \
        } \
        template Patch \
        { \
            <A3EB5D44-FC22-429D-9AFB-3221CB9719A6> \
            DWORD nControlIndices; \
            array DWORD controlIndices[nControlIndices]; \
        } \
        template PatchMesh \
        { \
            <D02C95CC-EDBA-4305-9B5D-1820D7704BBF> \
            DWORD nVertices; \
            array Vector vertices[nVertices]; \
            DWORD nPatches; \
            array Patch patches[nPatches]; \
            [ ... ] \
        } \
        template PatchMesh9 \
        { \
            <B9EC94E1-B9A6-4251-BA18-94893F02C0EA> \
            DWORD Type; \
            DWORD Degree; \
            DWORD Basis; \
            DWORD nVertices; \
            array Vector vertices[nVertices]; \
            DWORD nPatches; \
            array Patch patches[nPatches]; \
            [ ... ] \
        } " \
        "template EffectFloats \
        { \
            <F1CFE2B3-0DE3-4e28-AFA1-155A750A282D> \
            DWORD nFloats; \
            array float Floats[nFloats]; \
        } \
        template EffectString \
        { \
            <D55B097E-BDB6-4c52-B03D-6051C89D0E42> \
            STRING Value; \
        } \
        template EffectDWord \
        { \
            <622C0ED0-956E-4da9-908A-2AF94F3CE716> \
            DWORD Value; \
        } " \
        "template EffectParamFloats \
        { \
            <3014B9A0-62F5-478c-9B86-E4AC9F4E418B> \
            STRING ParamName; \
            DWORD nFloats; \
            array float Floats[nFloats]; \
        } " \
        "template EffectParamString \
        { \
            <1DBC4C88-94C1-46ee-9076-2C28818C9481> \
            STRING ParamName; \
            STRING Value; \
        } \
        template EffectParamDWord \
        { \
            <E13963BC-AE51-4c5d-B00F-CFA3A9D97CE5> \
            STRING ParamName; \
            DWORD Value; \
        } \
        template EffectInstance \
        { \
            <E331F7E4-0559-4cc2-8E99-1CEC1657928F> \
            STRING EffectFilename; \
            [ ... ] \
        } " \
        "template AnimTicksPerSecond \
        { \
            <9E415A43-7BA6-4a73-8743-B73D47E88476> \
            DWORD AnimTicksPerSecond; \
        } \
        template CompressedAnimationSet \
        { \
            <7F9B00B3-F125-4890-876E-1C42BF697C4D> \
            DWORD CompressedBlockSize; \
            FLOAT TicksPerSec; \
            DWORD PlaybackType; \
            DWORD BufferLength; \
            array DWORD CompressedData[BufferLength]; \
        } "

#define XEXTENSIONS_TEMPLATES \
        "xof 0303txt 0032\
        template FVFData \
        { \
            <B6E70A0E-8EF9-4e83-94AD-ECC8B0C04897> \
            DWORD dwFVF; \
            DWORD nDWords; \
            array DWORD data[nDWords]; \
        } \
        template VertexElement \
        { \
            <F752461C-1E23-48f6-B9F8-8350850F336F> \
            DWORD Type; \
            DWORD Method; \
            DWORD Usage; \
            DWORD UsageIndex; \
        } \
        template DeclData \
        { \
            <BF22E553-292C-4781-9FEA-62BD554BDD93> \
            DWORD nElements; \
            array VertexElement Elements[nElements]; \
            DWORD nDWords; \
            array DWORD data[nDWords]; \
        } \
        template PMAttributeRange \
        { \
            <917E0427-C61E-4a14-9C64-AFE65F9E9844> \
            DWORD iFaceOffset; \
            DWORD nFacesMin; \
            DWORD nFacesMax; \
            DWORD iVertexOffset; \
            DWORD nVerticesMin; \
            DWORD nVerticesMax; \
        } \
        template PMVSplitRecord \
        { \
            <574CCC14-F0B3-4333-822D-93E8A8A08E4C> \
            DWORD iFaceCLW; \
            DWORD iVlrOffset; \
            DWORD iCode; \
        } \
        template PMInfo \
        { \
            <B6C3E656-EC8B-4b92-9B62-681659522947> \
            DWORD nAttributes; \
            array PMAttributeRange attributeRanges[nAttributes]; \
            DWORD nMaxValence; \
            DWORD nMinLogicalVertices; \
            DWORD nMaxLogicalVertices; \
            DWORD nVSplits; \
            array PMVSplitRecord splitRecords[nVSplits]; \
            DWORD nAttributeMispredicts; \
            array DWORD attributeMispredicts[nAttributeMispredicts]; \
        } "
        
#endif //__D3DX9MESH_H__



//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  File:       d3dx9shader.h
//  Content:    D3DX Shader APIs
//
//////////////////////////////////////////////////////////////////////////////

#ifndef __D3DX9SHADER_H__
#define __D3DX9SHADER_H__


//---------------------------------------------------------------------------
// D3DXTX_VERSION:
// --------------
// Version token used to create a procedural texture filler in effects
// Used by D3DXFill[]TX functions
//---------------------------------------------------------------------------
#define D3DXTX_VERSION(_Major,_Minor) (('T' << 24) | ('X' << 16) | ((_Major) << 8) | (_Minor))



//----------------------------------------------------------------------------
// D3DXSHADER flags:
// -----------------
// D3DXSHADER_DEBUG
//   Insert debug file/line/type/symbol information.
//
// D3DXSHADER_SKIPVALIDATION
//   Do not validate the generated code against known capabilities and
//   constraints.  This option is only recommended when compiling shaders
//   you KNOW will work.  (ie. have compiled before without this option.)
//   Shaders are always validated by D3D before they are set to the device.
//
// D3DXSHADER_SKIPOPTIMIZATION 
//   Instructs the compiler to skip optimization steps during code generation.
//   Unless you are trying to isolate a problem in your code using this option 
//   is not recommended.
//
// D3DXSHADER_PACKMATRIX_ROWMAJOR
//   Unless explicitly specified, matrices will be packed in row-major order
//   on input and output from the shader.
//
// D3DXSHADER_PACKMATRIX_COLUMNMAJOR
//   Unless explicitly specified, matrices will be packed in column-major 
//   order on input and output from the shader.  This is generally more 
//   efficient, since it allows vector-matrix multiplication to be performed
//   using a series of dot-products.
//
// D3DXSHADER_PARTIALPRECISION
//   Force all computations in resulting shader to occur at partial precision.
//   This may result in faster evaluation of shaders on some hardware.
//
// D3DXSHADER_FORCE_VS_SOFTWARE_NOOPT
//   Force compiler to compile against the next highest available software
//   target for vertex shaders.  This flag also turns optimizations off, 
//   and debugging on.  
//
// D3DXSHADER_FORCE_PS_SOFTWARE_NOOPT
//   Force compiler to compile against the next highest available software
//   target for pixel shaders.  This flag also turns optimizations off, 
//   and debugging on.
//
// D3DXSHADER_NO_PRESHADER
//   Disables Preshaders. Using this flag will cause the compiler to not 
//   pull out static expression for evaluation on the host cpu
//
// D3DXSHADER_AVOID_FLOW_CONTROL
//   Hint compiler to avoid flow-control constructs where possible.
//
// D3DXSHADER_PREFER_FLOW_CONTROL
//   Hint compiler to prefer flow-control constructs where possible.
//
//----------------------------------------------------------------------------

#define D3DXSHADER_DEBUG                          (1 << 0)
#define D3DXSHADER_SKIPVALIDATION                 (1 << 1)
#define D3DXSHADER_SKIPOPTIMIZATION               (1 << 2)
#define D3DXSHADER_PACKMATRIX_ROWMAJOR            (1 << 3)
#define D3DXSHADER_PACKMATRIX_COLUMNMAJOR         (1 << 4)
#define D3DXSHADER_PARTIALPRECISION               (1 << 5)
#define D3DXSHADER_FORCE_VS_SOFTWARE_NOOPT        (1 << 6)
#define D3DXSHADER_FORCE_PS_SOFTWARE_NOOPT        (1 << 7)
#define D3DXSHADER_NO_PRESHADER                   (1 << 8)
#define D3DXSHADER_AVOID_FLOW_CONTROL             (1 << 9)
#define D3DXSHADER_PREFER_FLOW_CONTROL            (1 << 10)
#define D3DXSHADER_ENABLE_BACKWARDS_COMPATIBILITY (1 << 12)
#define D3DXSHADER_IEEE_STRICTNESS                (1 << 13)
#define D3DXSHADER_USE_LEGACY_D3DX9_31_DLL        (1 << 16)


// optimization level flags
#define D3DXSHADER_OPTIMIZATION_LEVEL0            (1 << 14)
#define D3DXSHADER_OPTIMIZATION_LEVEL1            0
#define D3DXSHADER_OPTIMIZATION_LEVEL2            ((1 << 14) | (1 << 15))
#define D3DXSHADER_OPTIMIZATION_LEVEL3            (1 << 15)



//----------------------------------------------------------------------------
// D3DXCONSTTABLE flags:
// -------------------

#define D3DXCONSTTABLE_LARGEADDRESSAWARE          (1 << 17)



//----------------------------------------------------------------------------
// D3DXHANDLE:
// -----------
// Handle values used to efficiently reference shader and effect parameters.
// Strings can be used as handles.  However, handles are not always strings.
//----------------------------------------------------------------------------

#ifndef D3DXFX_LARGEADDRESS_HANDLE
typedef LPCSTR D3DXHANDLE;
#else
typedef UINT_PTR D3DXHANDLE;
#endif
typedef D3DXHANDLE *LPD3DXHANDLE;


//----------------------------------------------------------------------------
// D3DXMACRO:
// ----------
// Preprocessor macro definition.  The application pass in a NULL-terminated
// array of this structure to various D3DX APIs.  This enables the application
// to #define tokens at runtime, before the file is parsed.
//----------------------------------------------------------------------------

typedef struct _D3DXMACRO
{
    LPCSTR Name;
    LPCSTR Definition;

} D3DXMACRO, *LPD3DXMACRO;


//----------------------------------------------------------------------------
// D3DXSEMANTIC:
//----------------------------------------------------------------------------

typedef struct _D3DXSEMANTIC
{
    UINT Usage;
    UINT UsageIndex;

} D3DXSEMANTIC, *LPD3DXSEMANTIC;



//----------------------------------------------------------------------------
// D3DXREGISTER_SET:
//----------------------------------------------------------------------------

typedef enum _D3DXREGISTER_SET
{
    D3DXRS_BOOL,
    D3DXRS_INT4,
    D3DXRS_FLOAT4,
    D3DXRS_SAMPLER,

    // force 32-bit size enum
    D3DXRS_FORCE_DWORD = 0x7fffffff

} D3DXREGISTER_SET, *LPD3DXREGISTER_SET;


//----------------------------------------------------------------------------
// D3DXPARAMETER_CLASS:
//----------------------------------------------------------------------------

typedef enum _D3DXPARAMETER_CLASS
{
    D3DXPC_SCALAR,
    D3DXPC_VECTOR,
    D3DXPC_MATRIX_ROWS,
    D3DXPC_MATRIX_COLUMNS,
    D3DXPC_OBJECT,
    D3DXPC_STRUCT,

    // force 32-bit size enum
    D3DXPC_FORCE_DWORD = 0x7fffffff

} D3DXPARAMETER_CLASS, *LPD3DXPARAMETER_CLASS;


//----------------------------------------------------------------------------
// D3DXPARAMETER_TYPE:
//----------------------------------------------------------------------------

typedef enum _D3DXPARAMETER_TYPE
{
    D3DXPT_VOID,
    D3DXPT_BOOL,
    D3DXPT_INT,
    D3DXPT_FLOAT,
    D3DXPT_STRING,
    D3DXPT_TEXTURE,
    D3DXPT_TEXTURE1D,
    D3DXPT_TEXTURE2D,
    D3DXPT_TEXTURE3D,
    D3DXPT_TEXTURECUBE,
    D3DXPT_SAMPLER,
    D3DXPT_SAMPLER1D,
    D3DXPT_SAMPLER2D,
    D3DXPT_SAMPLER3D,
    D3DXPT_SAMPLERCUBE,
    D3DXPT_PIXELSHADER,
    D3DXPT_VERTEXSHADER,
    D3DXPT_PIXELFRAGMENT,
    D3DXPT_VERTEXFRAGMENT,
    D3DXPT_UNSUPPORTED,

    // force 32-bit size enum
    D3DXPT_FORCE_DWORD = 0x7fffffff

} D3DXPARAMETER_TYPE, *LPD3DXPARAMETER_TYPE;


//----------------------------------------------------------------------------
// D3DXCONSTANTTABLE_DESC:
//----------------------------------------------------------------------------

typedef struct _D3DXCONSTANTTABLE_DESC
{
    LPCSTR Creator;                     // Creator string
    DWORD Version;                      // Shader version
    UINT Constants;                     // Number of constants

} D3DXCONSTANTTABLE_DESC, *LPD3DXCONSTANTTABLE_DESC;


//----------------------------------------------------------------------------
// D3DXCONSTANT_DESC:
//----------------------------------------------------------------------------

typedef struct _D3DXCONSTANT_DESC
{
    LPCSTR Name;                        // Constant name

    D3DXREGISTER_SET RegisterSet;       // Register set
    UINT RegisterIndex;                 // Register index
    UINT RegisterCount;                 // Number of registers occupied

    D3DXPARAMETER_CLASS Class;          // Class
    D3DXPARAMETER_TYPE Type;            // Component type

    UINT Rows;                          // Number of rows
    UINT Columns;                       // Number of columns
    UINT Elements;                      // Number of array elements
    UINT StructMembers;                 // Number of structure member sub-parameters

    UINT Bytes;                         // Data size, in bytes
    LPCVOID DefaultValue;               // Pointer to default value

} D3DXCONSTANT_DESC, *LPD3DXCONSTANT_DESC;



//----------------------------------------------------------------------------
// ID3DXConstantTable:
//----------------------------------------------------------------------------

typedef interface ID3DXConstantTable ID3DXConstantTable;
typedef interface ID3DXConstantTable *LPD3DXCONSTANTTABLE;

// {AB3C758F-093E-4356-B762-4DB18F1B3A01}
DEFINE_GUID(IID_ID3DXConstantTable, 
0xab3c758f, 0x93e, 0x4356, 0xb7, 0x62, 0x4d, 0xb1, 0x8f, 0x1b, 0x3a, 0x1);


#undef INTERFACE
#define INTERFACE ID3DXConstantTable

DECLARE_INTERFACE_(ID3DXConstantTable, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // Buffer
    STDMETHOD_(LPVOID, GetBufferPointer)(THIS) PURE;
    STDMETHOD_(DWORD, GetBufferSize)(THIS) PURE;

    // Descs
    STDMETHOD(GetDesc)(THIS_ D3DXCONSTANTTABLE_DESC *pDesc) PURE;
    STDMETHOD(GetConstantDesc)(THIS_ D3DXHANDLE hConstant, D3DXCONSTANT_DESC *pConstantDesc, UINT *pCount) PURE;
    STDMETHOD_(UINT, GetSamplerIndex)(THIS_ D3DXHANDLE hConstant) PURE;

    // Handle operations
    STDMETHOD_(D3DXHANDLE, GetConstant)(THIS_ D3DXHANDLE hConstant, UINT Index) PURE;
    STDMETHOD_(D3DXHANDLE, GetConstantByName)(THIS_ D3DXHANDLE hConstant, LPCSTR pName) PURE;
    STDMETHOD_(D3DXHANDLE, GetConstantElement)(THIS_ D3DXHANDLE hConstant, UINT Index) PURE;

    // Set Constants
    STDMETHOD(SetDefaults)(THIS_ LPDIRECT3DDEVICE9 pDevice) PURE;
    STDMETHOD(SetValue)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, LPCVOID pData, UINT Bytes) PURE;
    STDMETHOD(SetBool)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, BOOL b) PURE;
    STDMETHOD(SetBoolArray)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST BOOL* pb, UINT Count) PURE;
    STDMETHOD(SetInt)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, INT n) PURE;
    STDMETHOD(SetIntArray)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST INT* pn, UINT Count) PURE;
    STDMETHOD(SetFloat)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, FLOAT f) PURE;
    STDMETHOD(SetFloatArray)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST FLOAT* pf, UINT Count) PURE;
    STDMETHOD(SetVector)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST D3DXVECTOR4* pVector) PURE;
    STDMETHOD(SetVectorArray)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST D3DXVECTOR4* pVector, UINT Count) PURE;
    STDMETHOD(SetMatrix)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST D3DXMATRIX* pMatrix) PURE;
    STDMETHOD(SetMatrixArray)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST D3DXMATRIX* pMatrix, UINT Count) PURE;
    STDMETHOD(SetMatrixPointerArray)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST D3DXMATRIX** ppMatrix, UINT Count) PURE;
    STDMETHOD(SetMatrixTranspose)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST D3DXMATRIX* pMatrix) PURE;
    STDMETHOD(SetMatrixTransposeArray)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST D3DXMATRIX* pMatrix, UINT Count) PURE;
    STDMETHOD(SetMatrixTransposePointerArray)(THIS_ LPDIRECT3DDEVICE9 pDevice, D3DXHANDLE hConstant, CONST D3DXMATRIX** ppMatrix, UINT Count) PURE;
};


//----------------------------------------------------------------------------
// ID3DXTextureShader:
//----------------------------------------------------------------------------

typedef interface ID3DXTextureShader ID3DXTextureShader;
typedef interface ID3DXTextureShader *LPD3DXTEXTURESHADER;

// {3E3D67F8-AA7A-405d-A857-BA01D4758426}
DEFINE_GUID(IID_ID3DXTextureShader, 
0x3e3d67f8, 0xaa7a, 0x405d, 0xa8, 0x57, 0xba, 0x1, 0xd4, 0x75, 0x84, 0x26);

#undef INTERFACE
#define INTERFACE ID3DXTextureShader

DECLARE_INTERFACE_(ID3DXTextureShader, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // Gets
    STDMETHOD(GetFunction)(THIS_ LPD3DXBUFFER *ppFunction) PURE;
    STDMETHOD(GetConstantBuffer)(THIS_ LPD3DXBUFFER *ppConstantBuffer) PURE;

    // Descs
    STDMETHOD(GetDesc)(THIS_ D3DXCONSTANTTABLE_DESC *pDesc) PURE;
    STDMETHOD(GetConstantDesc)(THIS_ D3DXHANDLE hConstant, D3DXCONSTANT_DESC *pConstantDesc, UINT *pCount) PURE;

    // Handle operations
    STDMETHOD_(D3DXHANDLE, GetConstant)(THIS_ D3DXHANDLE hConstant, UINT Index) PURE;
    STDMETHOD_(D3DXHANDLE, GetConstantByName)(THIS_ D3DXHANDLE hConstant, LPCSTR pName) PURE;
    STDMETHOD_(D3DXHANDLE, GetConstantElement)(THIS_ D3DXHANDLE hConstant, UINT Index) PURE;

    // Set Constants
    STDMETHOD(SetDefaults)(THIS) PURE;
    STDMETHOD(SetValue)(THIS_ D3DXHANDLE hConstant, LPCVOID pData, UINT Bytes) PURE;
    STDMETHOD(SetBool)(THIS_ D3DXHANDLE hConstant, BOOL b) PURE;
    STDMETHOD(SetBoolArray)(THIS_ D3DXHANDLE hConstant, CONST BOOL* pb, UINT Count) PURE;
    STDMETHOD(SetInt)(THIS_ D3DXHANDLE hConstant, INT n) PURE;
    STDMETHOD(SetIntArray)(THIS_ D3DXHANDLE hConstant, CONST INT* pn, UINT Count) PURE;
    STDMETHOD(SetFloat)(THIS_ D3DXHANDLE hConstant, FLOAT f) PURE;
    STDMETHOD(SetFloatArray)(THIS_ D3DXHANDLE hConstant, CONST FLOAT* pf, UINT Count) PURE;
    STDMETHOD(SetVector)(THIS_ D3DXHANDLE hConstant, CONST D3DXVECTOR4* pVector) PURE;
    STDMETHOD(SetVectorArray)(THIS_ D3DXHANDLE hConstant, CONST D3DXVECTOR4* pVector, UINT Count) PURE;
    STDMETHOD(SetMatrix)(THIS_ D3DXHANDLE hConstant, CONST D3DXMATRIX* pMatrix) PURE;
    STDMETHOD(SetMatrixArray)(THIS_ D3DXHANDLE hConstant, CONST D3DXMATRIX* pMatrix, UINT Count) PURE;
    STDMETHOD(SetMatrixPointerArray)(THIS_ D3DXHANDLE hConstant, CONST D3DXMATRIX** ppMatrix, UINT Count) PURE;
    STDMETHOD(SetMatrixTranspose)(THIS_ D3DXHANDLE hConstant, CONST D3DXMATRIX* pMatrix) PURE;
    STDMETHOD(SetMatrixTransposeArray)(THIS_ D3DXHANDLE hConstant, CONST D3DXMATRIX* pMatrix, UINT Count) PURE;
    STDMETHOD(SetMatrixTransposePointerArray)(THIS_ D3DXHANDLE hConstant, CONST D3DXMATRIX** ppMatrix, UINT Count) PURE;
};


//----------------------------------------------------------------------------
// D3DXINCLUDE_TYPE:
//----------------------------------------------------------------------------

typedef enum _D3DXINCLUDE_TYPE
{
    D3DXINC_LOCAL,
    D3DXINC_SYSTEM,

    // force 32-bit size enum
    D3DXINC_FORCE_DWORD = 0x7fffffff

} D3DXINCLUDE_TYPE, *LPD3DXINCLUDE_TYPE;


//----------------------------------------------------------------------------
// ID3DXInclude:
// -------------
// This interface is intended to be implemented by the application, and can
// be used by various D3DX APIs.  This enables application-specific handling
// of #include directives in source files.
//
// Open()
//    Opens an include file.  If successful, it should fill in ppData and
//    pBytes.  The data pointer returned must remain valid until Close is
//    subsequently called.  The name of the file is encoded in UTF-8 format.
// Close()
//    Closes an include file.  If Open was successful, Close is guaranteed
//    to be called before the API using this interface returns.
//----------------------------------------------------------------------------

typedef interface ID3DXInclude ID3DXInclude;
typedef interface ID3DXInclude *LPD3DXINCLUDE;

#undef INTERFACE
#define INTERFACE ID3DXInclude

DECLARE_INTERFACE(ID3DXInclude)
{
    STDMETHOD(Open)(THIS_ D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes) PURE;
    STDMETHOD(Close)(THIS_ LPCVOID pData) PURE;
};


//////////////////////////////////////////////////////////////////////////////
// APIs //////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


//----------------------------------------------------------------------------
// D3DXAssembleShader:
// -------------------
// Assembles a shader.
//
// Parameters:
//  pSrcFile
//      Source file name
//  hSrcModule
//      Module handle. if NULL, current module will be used
//  pSrcResource
//      Resource name in module
//  pSrcData
//      Pointer to source code
//  SrcDataLen
//      Size of source code, in bytes
//  pDefines
//      Optional NULL-terminated array of preprocessor macro definitions.
//  pInclude
//      Optional interface pointer to use for handling #include directives.
//      If this parameter is NULL, #includes will be honored when assembling
//      from file, and will error when assembling from resource or memory.
//  Flags
//      See D3DXSHADER_xxx flags
//  ppShader
//      Returns a buffer containing the created shader.  This buffer contains
//      the assembled shader code, as well as any embedded debug info.
//  ppErrorMsgs
//      Returns a buffer containing a listing of errors and warnings that were
//      encountered during assembly.  If you are running in a debugger,
//      these are the same messages you will see in your debug output.
//----------------------------------------------------------------------------


HRESULT WINAPI
    D3DXAssembleShaderFromFileA(
        LPCSTR                          pSrcFile,
        CONST D3DXMACRO*                pDefines,
        LPD3DXINCLUDE                   pInclude,
        DWORD                           Flags,
        LPD3DXBUFFER*                   ppShader,
        LPD3DXBUFFER*                   ppErrorMsgs);

HRESULT WINAPI
    D3DXAssembleShaderFromFileW(
        LPCWSTR                         pSrcFile,
        CONST D3DXMACRO*                pDefines,
        LPD3DXINCLUDE                   pInclude,
        DWORD                           Flags,
        LPD3DXBUFFER*                   ppShader,
        LPD3DXBUFFER*                   ppErrorMsgs);

#ifdef UNICODE
#define D3DXAssembleShaderFromFile D3DXAssembleShaderFromFileW
#else
#define D3DXAssembleShaderFromFile D3DXAssembleShaderFromFileA
#endif


HRESULT WINAPI
    D3DXAssembleShaderFromResourceA(
        HMODULE                         hSrcModule,
        LPCSTR                          pSrcResource,
        CONST D3DXMACRO*                pDefines,
        LPD3DXINCLUDE                   pInclude,
        DWORD                           Flags,
        LPD3DXBUFFER*                   ppShader,
        LPD3DXBUFFER*                   ppErrorMsgs);

HRESULT WINAPI
    D3DXAssembleShaderFromResourceW(
        HMODULE                         hSrcModule,
        LPCWSTR                         pSrcResource,
        CONST D3DXMACRO*                pDefines,
        LPD3DXINCLUDE                   pInclude,
        DWORD                           Flags,
        LPD3DXBUFFER*                   ppShader,
        LPD3DXBUFFER*                   ppErrorMsgs);

#ifdef UNICODE
#define D3DXAssembleShaderFromResource D3DXAssembleShaderFromResourceW
#else
#define D3DXAssembleShaderFromResource D3DXAssembleShaderFromResourceA
#endif


HRESULT WINAPI
    D3DXAssembleShader(
        LPCSTR                          pSrcData,
        UINT                            SrcDataLen,
        CONST D3DXMACRO*                pDefines,
        LPD3DXINCLUDE                   pInclude,
        DWORD                           Flags,
        LPD3DXBUFFER*                   ppShader,
        LPD3DXBUFFER*                   ppErrorMsgs);



//----------------------------------------------------------------------------
// D3DXCompileShader:
// ------------------
// Compiles a shader.
//
// Parameters:
//  pSrcFile
//      Source file name.
//  hSrcModule
//      Module handle. if NULL, current module will be used.
//  pSrcResource
//      Resource name in module.
//  pSrcData
//      Pointer to source code.
//  SrcDataLen
//      Size of source code, in bytes.
//  pDefines
//      Optional NULL-terminated array of preprocessor macro definitions.
//  pInclude
//      Optional interface pointer to use for handling #include directives.
//      If this parameter is NULL, #includes will be honored when compiling
//      from file, and will error when compiling from resource or memory.
//  pFunctionName
//      Name of the entrypoint function where execution should begin.
//  pProfile
//      Instruction set to be used when generating code.  Currently supported
//      profiles are "vs_1_1", "vs_2_0", "vs_2_a", "vs_2_sw", "ps_1_1", 
//      "ps_1_2", "ps_1_3", "ps_1_4", "ps_2_0", "ps_2_a", "ps_2_sw", "tx_1_0"
//  Flags
//      See D3DXSHADER_xxx flags.
//  ppShader
//      Returns a buffer containing the created shader.  This buffer contains
//      the compiled shader code, as well as any embedded debug and symbol
//      table info.  (See D3DXGetShaderConstantTable)
//  ppErrorMsgs
//      Returns a buffer containing a listing of errors and warnings that were
//      encountered during the compile.  If you are running in a debugger,
//      these are the same messages you will see in your debug output.
//  ppConstantTable
//      Returns a ID3DXConstantTable object which can be used to set
//      shader constants to the device.  Alternatively, an application can
//      parse the D3DXSHADER_CONSTANTTABLE block embedded as a comment within
//      the shader.
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXCompileShaderFromFileA(
        LPCSTR                          pSrcFile,
        CONST D3DXMACRO*                pDefines,
        LPD3DXINCLUDE                   pInclude,
        LPCSTR                          pFunctionName,
        LPCSTR                          pProfile,
        DWORD                           Flags,
        LPD3DXBUFFER*                   ppShader,
        LPD3DXBUFFER*                   ppErrorMsgs,
        LPD3DXCONSTANTTABLE*            ppConstantTable);

HRESULT WINAPI
    D3DXCompileShaderFromFileW(
        LPCWSTR                         pSrcFile,
        CONST D3DXMACRO*                pDefines,
        LPD3DXINCLUDE                   pInclude,
        LPCSTR                          pFunctionName,
        LPCSTR                          pProfile,
        DWORD                           Flags,
        LPD3DXBUFFER*                   ppShader,
        LPD3DXBUFFER*                   ppErrorMsgs,
        LPD3DXCONSTANTTABLE*            ppConstantTable);

#ifdef UNICODE
#define D3DXCompileShaderFromFile D3DXCompileShaderFromFileW
#else
#define D3DXCompileShaderFromFile D3DXCompileShaderFromFileA
#endif


HRESULT WINAPI
    D3DXCompileShaderFromResourceA(
        HMODULE                         hSrcModule,
        LPCSTR                          pSrcResource,
        CONST D3DXMACRO*                pDefines,
        LPD3DXINCLUDE                   pInclude,
        LPCSTR                          pFunctionName,
        LPCSTR                          pProfile,
        DWORD                           Flags,
        LPD3DXBUFFER*                   ppShader,
        LPD3DXBUFFER*                   ppErrorMsgs,
        LPD3DXCONSTANTTABLE*            ppConstantTable);

HRESULT WINAPI
    D3DXCompileShaderFromResourceW(
        HMODULE                         hSrcModule,
        LPCWSTR                         pSrcResource,
        CONST D3DXMACRO*                pDefines,
        LPD3DXINCLUDE                   pInclude,
        LPCSTR                          pFunctionName,
        LPCSTR                          pProfile,
        DWORD                           Flags,
        LPD3DXBUFFER*                   ppShader,
        LPD3DXBUFFER*                   ppErrorMsgs,
        LPD3DXCONSTANTTABLE*            ppConstantTable);

#ifdef UNICODE
#define D3DXCompileShaderFromResource D3DXCompileShaderFromResourceW
#else
#define D3DXCompileShaderFromResource D3DXCompileShaderFromResourceA
#endif


HRESULT WINAPI
    D3DXCompileShader(
        LPCSTR                          pSrcData,
        UINT                            SrcDataLen,
        CONST D3DXMACRO*                pDefines,
        LPD3DXINCLUDE                   pInclude,
        LPCSTR                          pFunctionName,
        LPCSTR                          pProfile,
        DWORD                           Flags,
        LPD3DXBUFFER*                   ppShader,
        LPD3DXBUFFER*                   ppErrorMsgs,
        LPD3DXCONSTANTTABLE*            ppConstantTable);


//----------------------------------------------------------------------------
// D3DXDisassembleShader:
// ----------------------
// Takes a binary shader, and returns a buffer containing text assembly.
//
// Parameters:
//  pShader
//      Pointer to the shader byte code.
//  ShaderSizeInBytes
//      Size of the shader byte code in bytes.
//  EnableColorCode
//      Emit HTML tags for color coding the output?
//  pComments
//      Pointer to a comment string to include at the top of the shader.
//  ppDisassembly
//      Returns a buffer containing the disassembled shader.
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXDisassembleShader(
        CONST DWORD*                    pShader, 
        BOOL                            EnableColorCode, 
        LPCSTR                          pComments, 
        LPD3DXBUFFER*                   ppDisassembly);


//----------------------------------------------------------------------------
// D3DXGetPixelShaderProfile/D3DXGetVertexShaderProfile:
// -----------------------------------------------------
// Returns the name of the HLSL profile best suited to a given device.
//
// Parameters:
//  pDevice
//      Pointer to the device in question
//----------------------------------------------------------------------------

LPCSTR WINAPI
    D3DXGetPixelShaderProfile(
        LPDIRECT3DDEVICE9               pDevice);

LPCSTR WINAPI
    D3DXGetVertexShaderProfile(
        LPDIRECT3DDEVICE9               pDevice);


//----------------------------------------------------------------------------
// D3DXFindShaderComment:
// ----------------------
// Searches through a shader for a particular comment, denoted by a FourCC in
// the first DWORD of the comment.  If the comment is not found, and no other
// error has occurred, S_FALSE is returned.
//
// Parameters:
//  pFunction
//      Pointer to the function DWORD stream
//  FourCC
//      FourCC used to identify the desired comment block.
//  ppData
//      Returns a pointer to the comment data (not including comment token
//      and FourCC).  Can be NULL.
//  pSizeInBytes
//      Returns the size of the comment data in bytes.  Can be NULL.
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXFindShaderComment(
        CONST DWORD*                    pFunction,
        DWORD                           FourCC,
        LPCVOID*                        ppData,
        UINT*                           pSizeInBytes);


//----------------------------------------------------------------------------
// D3DXGetShaderSize:
// ------------------
// Returns the size of the shader byte-code, in bytes.
//
// Parameters:
//  pFunction
//      Pointer to the function DWORD stream
//----------------------------------------------------------------------------

UINT WINAPI
    D3DXGetShaderSize(
        CONST DWORD*                    pFunction);


//----------------------------------------------------------------------------
// D3DXGetShaderVersion:
// -----------------------
// Returns the shader version of a given shader.  Returns zero if the shader 
// function is NULL.
//
// Parameters:
//  pFunction
//      Pointer to the function DWORD stream
//----------------------------------------------------------------------------

DWORD WINAPI
    D3DXGetShaderVersion(
        CONST DWORD*                    pFunction);

//----------------------------------------------------------------------------
// D3DXGetShaderSemantics:
// -----------------------
// Gets semantics for all input elements referenced inside a given shader.
//
// Parameters:
//  pFunction
//      Pointer to the function DWORD stream
//  pSemantics
//      Pointer to an array of D3DXSEMANTIC structures.  The function will
//      fill this array with the semantics for each input element referenced
//      inside the shader.  This array is assumed to contain at least
//      MAXD3DDECLLENGTH elements.
//  pCount
//      Returns the number of elements referenced by the shader
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXGetShaderInputSemantics(
        CONST DWORD*                    pFunction,
        D3DXSEMANTIC*                   pSemantics,
        UINT*                           pCount);

HRESULT WINAPI
    D3DXGetShaderOutputSemantics(
        CONST DWORD*                    pFunction,
        D3DXSEMANTIC*                   pSemantics,
        UINT*                           pCount);


//----------------------------------------------------------------------------
// D3DXGetShaderSamplers:
// ----------------------
// Gets semantics for all input elements referenced inside a given shader.
//
// pFunction
//      Pointer to the function DWORD stream
// pSamplers
//      Pointer to an array of LPCSTRs.  The function will fill this array
//      with pointers to the sampler names contained within pFunction, for
//      each sampler referenced inside the shader.  This array is assumed to
//      contain at least 16 elements.
// pCount
//      Returns the number of samplers referenced by the shader
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXGetShaderSamplers(
        CONST DWORD*                    pFunction,
        LPCSTR*                         pSamplers,
        UINT*                           pCount);


//----------------------------------------------------------------------------
// D3DXGetShaderConstantTable:
// ---------------------------
// Gets shader constant table embedded inside shader.  A constant table is
// generated by D3DXAssembleShader and D3DXCompileShader, and is embedded in
// the body of the shader.
//
// Parameters:
//  pFunction
//      Pointer to the function DWORD stream
//  Flags
//      See D3DXCONSTTABLE_xxx
//  ppConstantTable
//      Returns a ID3DXConstantTable object which can be used to set
//      shader constants to the device.  Alternatively, an application can
//      parse the D3DXSHADER_CONSTANTTABLE block embedded as a comment within
//      the shader.
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXGetShaderConstantTable(
        CONST DWORD*                    pFunction,
        LPD3DXCONSTANTTABLE*            ppConstantTable);

HRESULT WINAPI
    D3DXGetShaderConstantTableEx(
        CONST DWORD*                    pFunction,
        DWORD                           Flags,
        LPD3DXCONSTANTTABLE*            ppConstantTable);



//----------------------------------------------------------------------------
// D3DXCreateTextureShader:
// ------------------------
// Creates a texture shader object, given the compiled shader.
//
// Parameters
//  pFunction
//      Pointer to the function DWORD stream
//  ppTextureShader
//      Returns a ID3DXTextureShader object which can be used to procedurally 
//      fill the contents of a texture using the D3DXFillTextureTX functions.
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXCreateTextureShader(
        CONST DWORD*                    pFunction, 
        LPD3DXTEXTURESHADER*            ppTextureShader);


//----------------------------------------------------------------------------
// D3DXPreprocessShader:
// ---------------------
// Runs the preprocessor on the specified shader or effect, but does
// not actually compile it.  This is useful for evaluating the #includes
// and #defines in a shader and then emitting a reformatted token stream
// for debugging purposes or for generating a self-contained shader.
//
// Parameters:
//  pSrcFile
//      Source file name
//  hSrcModule
//      Module handle. if NULL, current module will be used
//  pSrcResource
//      Resource name in module
//  pSrcData
//      Pointer to source code
//  SrcDataLen
//      Size of source code, in bytes
//  pDefines
//      Optional NULL-terminated array of preprocessor macro definitions.
//  pInclude
//      Optional interface pointer to use for handling #include directives.
//      If this parameter is NULL, #includes will be honored when assembling
//      from file, and will error when assembling from resource or memory.
//  ppShaderText
//      Returns a buffer containing a single large string that represents
//      the resulting formatted token stream
//  ppErrorMsgs
//      Returns a buffer containing a listing of errors and warnings that were
//      encountered during assembly.  If you are running in a debugger,
//      these are the same messages you will see in your debug output.
//----------------------------------------------------------------------------

HRESULT WINAPI 
    D3DXPreprocessShaderFromFileA(
        LPCSTR                       pSrcFile,
        CONST D3DXMACRO*             pDefines,
        LPD3DXINCLUDE                pInclude,
        LPD3DXBUFFER*                ppShaderText,
        LPD3DXBUFFER*                ppErrorMsgs);
                                             
HRESULT WINAPI 
    D3DXPreprocessShaderFromFileW(
        LPCWSTR                      pSrcFile,
        CONST D3DXMACRO*             pDefines,
        LPD3DXINCLUDE                pInclude,
        LPD3DXBUFFER*                ppShaderText,
        LPD3DXBUFFER*                ppErrorMsgs);

#ifdef UNICODE
#define D3DXPreprocessShaderFromFile D3DXPreprocessShaderFromFileW
#else
#define D3DXPreprocessShaderFromFile D3DXPreprocessShaderFromFileA
#endif
                                             
HRESULT WINAPI 
    D3DXPreprocessShaderFromResourceA(
        HMODULE                      hSrcModule,
        LPCSTR                       pSrcResource,
        CONST D3DXMACRO*             pDefines,
        LPD3DXINCLUDE                pInclude,
        LPD3DXBUFFER*                ppShaderText,
        LPD3DXBUFFER*                ppErrorMsgs);

HRESULT WINAPI 
    D3DXPreprocessShaderFromResourceW(
        HMODULE                      hSrcModule,
        LPCWSTR                      pSrcResource,
        CONST D3DXMACRO*             pDefines,
        LPD3DXINCLUDE                pInclude,
        LPD3DXBUFFER*                ppShaderText,
        LPD3DXBUFFER*                ppErrorMsgs);

#ifdef UNICODE
#define D3DXPreprocessShaderFromResource D3DXPreprocessShaderFromResourceW
#else
#define D3DXPreprocessShaderFromResource D3DXPreprocessShaderFromResourceA
#endif

HRESULT WINAPI 
    D3DXPreprocessShader(
        LPCSTR                       pSrcData,
        UINT                         SrcDataSize,
        CONST D3DXMACRO*             pDefines,
        LPD3DXINCLUDE                pInclude,
        LPD3DXBUFFER*                ppShaderText,
        LPD3DXBUFFER*                ppErrorMsgs);


#ifdef __cplusplus
}
#endif //__cplusplus


//////////////////////////////////////////////////////////////////////////////
// Shader comment block layouts //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// D3DXSHADER_CONSTANTTABLE:
// -------------------------
// Shader constant information; included as an CTAB comment block inside
// shaders.  All offsets are BYTE offsets from start of CONSTANTTABLE struct.
// Entries in the table are sorted by Name in ascending order.
//----------------------------------------------------------------------------

typedef struct _D3DXSHADER_CONSTANTTABLE
{
    DWORD Size;             // sizeof(D3DXSHADER_CONSTANTTABLE)
    DWORD Creator;          // LPCSTR offset
    DWORD Version;          // shader version
    DWORD Constants;        // number of constants
    DWORD ConstantInfo;     // D3DXSHADER_CONSTANTINFO[Constants] offset
    DWORD Flags;            // flags shader was compiled with
    DWORD Target;           // LPCSTR offset 

} D3DXSHADER_CONSTANTTABLE, *LPD3DXSHADER_CONSTANTTABLE;


typedef struct _D3DXSHADER_CONSTANTINFO
{
    DWORD Name;             // LPCSTR offset
    WORD  RegisterSet;      // D3DXREGISTER_SET
    WORD  RegisterIndex;    // register number
    WORD  RegisterCount;    // number of registers
    WORD  Reserved;         // reserved
    DWORD TypeInfo;         // D3DXSHADER_TYPEINFO offset
    DWORD DefaultValue;     // offset of default value

} D3DXSHADER_CONSTANTINFO, *LPD3DXSHADER_CONSTANTINFO;


typedef struct _D3DXSHADER_TYPEINFO
{
    WORD  Class;            // D3DXPARAMETER_CLASS
    WORD  Type;             // D3DXPARAMETER_TYPE
    WORD  Rows;             // number of rows (matrices)
    WORD  Columns;          // number of columns (vectors and matrices)
    WORD  Elements;         // array dimension
    WORD  StructMembers;    // number of struct members
    DWORD StructMemberInfo; // D3DXSHADER_STRUCTMEMBERINFO[Members] offset

} D3DXSHADER_TYPEINFO, *LPD3DXSHADER_TYPEINFO;


typedef struct _D3DXSHADER_STRUCTMEMBERINFO
{
    DWORD Name;             // LPCSTR offset
    DWORD TypeInfo;         // D3DXSHADER_TYPEINFO offset

} D3DXSHADER_STRUCTMEMBERINFO, *LPD3DXSHADER_STRUCTMEMBERINFO;



#endif //__D3DX9SHADER_H__



//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  File:       d3dx9effect.h
//  Content:    D3DX effect types and Shaders
//
//////////////////////////////////////////////////////////////////////////////

#ifndef __D3DX9EFFECT_H__
#define __D3DX9EFFECT_H__


//----------------------------------------------------------------------------
// D3DXFX_DONOTSAVESTATE
//   This flag is used as a parameter to ID3DXEffect::Begin().  When this flag
//   is specified, device state is not saved or restored in Begin/End.
// D3DXFX_DONOTSAVESHADERSTATE
//   This flag is used as a parameter to ID3DXEffect::Begin().  When this flag
//   is specified, shader device state is not saved or restored in Begin/End.
//   This includes pixel/vertex shaders and shader constants
// D3DXFX_DONOTSAVESAMPLERSTATE
//   This flag is used as a parameter to ID3DXEffect::Begin(). When this flag
//   is specified, sampler device state is not saved or restored in Begin/End.
// D3DXFX_NOT_CLONEABLE
//   This flag is used as a parameter to the D3DXCreateEffect family of APIs.
//   When this flag is specified, the effect will be non-cloneable and will not
//   contain any shader binary data.
//   Furthermore, GetPassDesc will not return shader function pointers. 
//   Setting this flag reduces effect memory usage by about 50%.
//----------------------------------------------------------------------------

#define D3DXFX_DONOTSAVESTATE         (1 << 0)
#define D3DXFX_DONOTSAVESHADERSTATE   (1 << 1)
#define D3DXFX_DONOTSAVESAMPLERSTATE  (1 << 2)

#define D3DXFX_NOT_CLONEABLE          (1 << 11)
#define D3DXFX_LARGEADDRESSAWARE      (1 << 17)

//----------------------------------------------------------------------------
// D3DX_PARAMETER_SHARED
//   Indicates that the value of a parameter will be shared with all effects
//   which share the same namespace.  Changing the value in one effect will
//   change it in all.
//
// D3DX_PARAMETER_LITERAL
//   Indicates that the value of this parameter can be treated as literal.
//   Literal parameters can be marked when the effect is compiled, and their
//   cannot be changed after the effect is compiled.  Shared parameters cannot
//   be literal.
//----------------------------------------------------------------------------

#define D3DX_PARAMETER_SHARED       (1 << 0)
#define D3DX_PARAMETER_LITERAL      (1 << 1)
#define D3DX_PARAMETER_ANNOTATION   (1 << 2)

//----------------------------------------------------------------------------
// D3DXEFFECT_DESC:
//----------------------------------------------------------------------------

typedef struct _D3DXEFFECT_DESC
{
    LPCSTR Creator;                     // Creator string
    UINT Parameters;                    // Number of parameters
    UINT Techniques;                    // Number of techniques
    UINT Functions;                     // Number of function entrypoints

} D3DXEFFECT_DESC;


//----------------------------------------------------------------------------
// D3DXPARAMETER_DESC:
//----------------------------------------------------------------------------

typedef struct _D3DXPARAMETER_DESC
{
    LPCSTR Name;                        // Parameter name
    LPCSTR Semantic;                    // Parameter semantic
    D3DXPARAMETER_CLASS Class;          // Class
    D3DXPARAMETER_TYPE Type;            // Component type
    UINT Rows;                          // Number of rows
    UINT Columns;                       // Number of columns
    UINT Elements;                      // Number of array elements
    UINT Annotations;                   // Number of annotations
    UINT StructMembers;                 // Number of structure member sub-parameters
    DWORD Flags;                        // D3DX_PARAMETER_* flags
    UINT Bytes;                         // Parameter size, in bytes

} D3DXPARAMETER_DESC;


//----------------------------------------------------------------------------
// D3DXTECHNIQUE_DESC:
//----------------------------------------------------------------------------

typedef struct _D3DXTECHNIQUE_DESC
{
    LPCSTR Name;                        // Technique name
    UINT Passes;                        // Number of passes
    UINT Annotations;                   // Number of annotations

} D3DXTECHNIQUE_DESC;


//----------------------------------------------------------------------------
// D3DXPASS_DESC:
//----------------------------------------------------------------------------

typedef struct _D3DXPASS_DESC
{
    LPCSTR Name;                        // Pass name
    UINT Annotations;                   // Number of annotations

    CONST DWORD *pVertexShaderFunction; // Vertex shader function
    CONST DWORD *pPixelShaderFunction;  // Pixel shader function

} D3DXPASS_DESC;


//----------------------------------------------------------------------------
// D3DXFUNCTION_DESC:
//----------------------------------------------------------------------------

typedef struct _D3DXFUNCTION_DESC
{
    LPCSTR Name;                        // Function name
    UINT Annotations;                   // Number of annotations

} D3DXFUNCTION_DESC;



//////////////////////////////////////////////////////////////////////////////
// ID3DXEffectPool ///////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef interface ID3DXEffectPool ID3DXEffectPool;
typedef interface ID3DXEffectPool *LPD3DXEFFECTPOOL;

// {9537AB04-3250-412e-8213-FCD2F8677933}
DEFINE_GUID(IID_ID3DXEffectPool, 
0x9537ab04, 0x3250, 0x412e, 0x82, 0x13, 0xfc, 0xd2, 0xf8, 0x67, 0x79, 0x33);


#undef INTERFACE
#define INTERFACE ID3DXEffectPool

DECLARE_INTERFACE_(ID3DXEffectPool, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // No public methods
};


//////////////////////////////////////////////////////////////////////////////
// ID3DXBaseEffect ///////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef interface ID3DXBaseEffect ID3DXBaseEffect;
typedef interface ID3DXBaseEffect *LPD3DXBASEEFFECT;

// {017C18AC-103F-4417-8C51-6BF6EF1E56BE}
DEFINE_GUID(IID_ID3DXBaseEffect, 
0x17c18ac, 0x103f, 0x4417, 0x8c, 0x51, 0x6b, 0xf6, 0xef, 0x1e, 0x56, 0xbe);


#undef INTERFACE
#define INTERFACE ID3DXBaseEffect

DECLARE_INTERFACE_(ID3DXBaseEffect, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // Descs
    STDMETHOD(GetDesc)(THIS_ D3DXEFFECT_DESC* pDesc) PURE;
    STDMETHOD(GetParameterDesc)(THIS_ D3DXHANDLE hParameter, D3DXPARAMETER_DESC* pDesc) PURE;
    STDMETHOD(GetTechniqueDesc)(THIS_ D3DXHANDLE hTechnique, D3DXTECHNIQUE_DESC* pDesc) PURE;
    STDMETHOD(GetPassDesc)(THIS_ D3DXHANDLE hPass, D3DXPASS_DESC* pDesc) PURE;
    STDMETHOD(GetFunctionDesc)(THIS_ D3DXHANDLE hShader, D3DXFUNCTION_DESC* pDesc) PURE;

    // Handle operations
    STDMETHOD_(D3DXHANDLE, GetParameter)(THIS_ D3DXHANDLE hParameter, UINT Index) PURE;
    STDMETHOD_(D3DXHANDLE, GetParameterByName)(THIS_ D3DXHANDLE hParameter, LPCSTR pName) PURE;
    STDMETHOD_(D3DXHANDLE, GetParameterBySemantic)(THIS_ D3DXHANDLE hParameter, LPCSTR pSemantic) PURE;
    STDMETHOD_(D3DXHANDLE, GetParameterElement)(THIS_ D3DXHANDLE hParameter, UINT Index) PURE;
    STDMETHOD_(D3DXHANDLE, GetTechnique)(THIS_ UINT Index) PURE;
    STDMETHOD_(D3DXHANDLE, GetTechniqueByName)(THIS_ LPCSTR pName) PURE;
    STDMETHOD_(D3DXHANDLE, GetPass)(THIS_ D3DXHANDLE hTechnique, UINT Index) PURE;
    STDMETHOD_(D3DXHANDLE, GetPassByName)(THIS_ D3DXHANDLE hTechnique, LPCSTR pName) PURE;
    STDMETHOD_(D3DXHANDLE, GetFunction)(THIS_ UINT Index) PURE;
    STDMETHOD_(D3DXHANDLE, GetFunctionByName)(THIS_ LPCSTR pName) PURE;
    STDMETHOD_(D3DXHANDLE, GetAnnotation)(THIS_ D3DXHANDLE hObject, UINT Index) PURE;
    STDMETHOD_(D3DXHANDLE, GetAnnotationByName)(THIS_ D3DXHANDLE hObject, LPCSTR pName) PURE;

    // Get/Set Parameters
    STDMETHOD(SetValue)(THIS_ D3DXHANDLE hParameter, LPCVOID pData, UINT Bytes) PURE;
    STDMETHOD(GetValue)(THIS_ D3DXHANDLE hParameter, LPVOID pData, UINT Bytes) PURE;
    STDMETHOD(SetBool)(THIS_ D3DXHANDLE hParameter, BOOL b) PURE;
    STDMETHOD(GetBool)(THIS_ D3DXHANDLE hParameter, BOOL* pb) PURE;
    STDMETHOD(SetBoolArray)(THIS_ D3DXHANDLE hParameter, CONST BOOL* pb, UINT Count) PURE;
    STDMETHOD(GetBoolArray)(THIS_ D3DXHANDLE hParameter, BOOL* pb, UINT Count) PURE;
    STDMETHOD(SetInt)(THIS_ D3DXHANDLE hParameter, INT n) PURE;
    STDMETHOD(GetInt)(THIS_ D3DXHANDLE hParameter, INT* pn) PURE;
    STDMETHOD(SetIntArray)(THIS_ D3DXHANDLE hParameter, CONST INT* pn, UINT Count) PURE;
    STDMETHOD(GetIntArray)(THIS_ D3DXHANDLE hParameter, INT* pn, UINT Count) PURE;
    STDMETHOD(SetFloat)(THIS_ D3DXHANDLE hParameter, FLOAT f) PURE;
    STDMETHOD(GetFloat)(THIS_ D3DXHANDLE hParameter, FLOAT* pf) PURE;
    STDMETHOD(SetFloatArray)(THIS_ D3DXHANDLE hParameter, CONST FLOAT* pf, UINT Count) PURE;
    STDMETHOD(GetFloatArray)(THIS_ D3DXHANDLE hParameter, FLOAT* pf, UINT Count) PURE;
    STDMETHOD(SetVector)(THIS_ D3DXHANDLE hParameter, CONST D3DXVECTOR4* pVector) PURE;
    STDMETHOD(GetVector)(THIS_ D3DXHANDLE hParameter, D3DXVECTOR4* pVector) PURE;
    STDMETHOD(SetVectorArray)(THIS_ D3DXHANDLE hParameter, CONST D3DXVECTOR4* pVector, UINT Count) PURE;
    STDMETHOD(GetVectorArray)(THIS_ D3DXHANDLE hParameter, D3DXVECTOR4* pVector, UINT Count) PURE;
    STDMETHOD(SetMatrix)(THIS_ D3DXHANDLE hParameter, CONST D3DXMATRIX* pMatrix) PURE;
    STDMETHOD(GetMatrix)(THIS_ D3DXHANDLE hParameter, D3DXMATRIX* pMatrix) PURE;
    STDMETHOD(SetMatrixArray)(THIS_ D3DXHANDLE hParameter, CONST D3DXMATRIX* pMatrix, UINT Count) PURE;
    STDMETHOD(GetMatrixArray)(THIS_ D3DXHANDLE hParameter, D3DXMATRIX* pMatrix, UINT Count) PURE;
    STDMETHOD(SetMatrixPointerArray)(THIS_ D3DXHANDLE hParameter, CONST D3DXMATRIX** ppMatrix, UINT Count) PURE;
    STDMETHOD(GetMatrixPointerArray)(THIS_ D3DXHANDLE hParameter, D3DXMATRIX** ppMatrix, UINT Count) PURE;
    STDMETHOD(SetMatrixTranspose)(THIS_ D3DXHANDLE hParameter, CONST D3DXMATRIX* pMatrix) PURE;
    STDMETHOD(GetMatrixTranspose)(THIS_ D3DXHANDLE hParameter, D3DXMATRIX* pMatrix) PURE;
    STDMETHOD(SetMatrixTransposeArray)(THIS_ D3DXHANDLE hParameter, CONST D3DXMATRIX* pMatrix, UINT Count) PURE;
    STDMETHOD(GetMatrixTransposeArray)(THIS_ D3DXHANDLE hParameter, D3DXMATRIX* pMatrix, UINT Count) PURE;
    STDMETHOD(SetMatrixTransposePointerArray)(THIS_ D3DXHANDLE hParameter, CONST D3DXMATRIX** ppMatrix, UINT Count) PURE;
    STDMETHOD(GetMatrixTransposePointerArray)(THIS_ D3DXHANDLE hParameter, D3DXMATRIX** ppMatrix, UINT Count) PURE;
    STDMETHOD(SetString)(THIS_ D3DXHANDLE hParameter, LPCSTR pString) PURE;
    STDMETHOD(GetString)(THIS_ D3DXHANDLE hParameter, LPCSTR* ppString) PURE;
    STDMETHOD(SetTexture)(THIS_ D3DXHANDLE hParameter, LPDIRECT3DBASETEXTURE9 pTexture) PURE;
    STDMETHOD(GetTexture)(THIS_ D3DXHANDLE hParameter, LPDIRECT3DBASETEXTURE9 *ppTexture) PURE;
    STDMETHOD(GetPixelShader)(THIS_ D3DXHANDLE hParameter, LPDIRECT3DPIXELSHADER9 *ppPShader) PURE;
    STDMETHOD(GetVertexShader)(THIS_ D3DXHANDLE hParameter, LPDIRECT3DVERTEXSHADER9 *ppVShader) PURE;

    //Set Range of an Array to pass to device
    //Useful for sending only a subrange of an array down to the device
    STDMETHOD(SetArrayRange)(THIS_ D3DXHANDLE hParameter, UINT uStart, UINT uEnd) PURE; 

};


//----------------------------------------------------------------------------
// ID3DXEffectStateManager:
// ------------------------
// This is a user implemented interface that can be used to manage device 
// state changes made by an Effect.
//----------------------------------------------------------------------------

typedef interface ID3DXEffectStateManager ID3DXEffectStateManager;
typedef interface ID3DXEffectStateManager *LPD3DXEFFECTSTATEMANAGER;

// {79AAB587-6DBC-4fa7-82DE-37FA1781C5CE}
DEFINE_GUID(IID_ID3DXEffectStateManager, 
0x79aab587, 0x6dbc, 0x4fa7, 0x82, 0xde, 0x37, 0xfa, 0x17, 0x81, 0xc5, 0xce);

#undef INTERFACE
#define INTERFACE ID3DXEffectStateManager

DECLARE_INTERFACE_(ID3DXEffectStateManager, IUnknown)
{
    // The user must correctly implement QueryInterface, AddRef, and Release.

    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // The following methods are called by the Effect when it wants to make 
    // the corresponding device call.  Note that:
    // 1. Users manage the state and are therefore responsible for making the 
    //    the corresponding device calls themselves inside their callbacks.  
    // 2. Effects pay attention to the return values of the callbacks, and so 
    //    users must pay attention to what they return in their callbacks.

    STDMETHOD(SetTransform)(THIS_ D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX *pMatrix) PURE;
    STDMETHOD(SetMaterial)(THIS_ CONST D3DMATERIAL9 *pMaterial) PURE;
    STDMETHOD(SetLight)(THIS_ DWORD Index, CONST D3DLIGHT9 *pLight) PURE;
    STDMETHOD(LightEnable)(THIS_ DWORD Index, BOOL Enable) PURE;
    STDMETHOD(SetRenderState)(THIS_ D3DRENDERSTATETYPE State, DWORD Value) PURE;
    STDMETHOD(SetTexture)(THIS_ DWORD Stage, LPDIRECT3DBASETEXTURE9 pTexture) PURE;
    STDMETHOD(SetTextureStageState)(THIS_ DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) PURE;
    STDMETHOD(SetSamplerState)(THIS_ DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value) PURE;
    STDMETHOD(SetNPatchMode)(THIS_ FLOAT NumSegments) PURE;
    STDMETHOD(SetFVF)(THIS_ DWORD FVF) PURE;
    STDMETHOD(SetVertexShader)(THIS_ LPDIRECT3DVERTEXSHADER9 pShader) PURE;
    STDMETHOD(SetVertexShaderConstantF)(THIS_ UINT RegisterIndex, CONST FLOAT *pConstantData, UINT RegisterCount) PURE;
    STDMETHOD(SetVertexShaderConstantI)(THIS_ UINT RegisterIndex, CONST INT *pConstantData, UINT RegisterCount) PURE;
    STDMETHOD(SetVertexShaderConstantB)(THIS_ UINT RegisterIndex, CONST BOOL *pConstantData, UINT RegisterCount) PURE;
    STDMETHOD(SetPixelShader)(THIS_ LPDIRECT3DPIXELSHADER9 pShader) PURE;
    STDMETHOD(SetPixelShaderConstantF)(THIS_ UINT RegisterIndex, CONST FLOAT *pConstantData, UINT RegisterCount) PURE;
    STDMETHOD(SetPixelShaderConstantI)(THIS_ UINT RegisterIndex, CONST INT *pConstantData, UINT RegisterCount) PURE;
    STDMETHOD(SetPixelShaderConstantB)(THIS_ UINT RegisterIndex, CONST BOOL *pConstantData, UINT RegisterCount) PURE;
};


//////////////////////////////////////////////////////////////////////////////
// ID3DXEffect ///////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef interface ID3DXEffect ID3DXEffect;
typedef interface ID3DXEffect *LPD3DXEFFECT;

// {F6CEB4B3-4E4C-40dd-B883-8D8DE5EA0CD5}
DEFINE_GUID(IID_ID3DXEffect, 
0xf6ceb4b3, 0x4e4c, 0x40dd, 0xb8, 0x83, 0x8d, 0x8d, 0xe5, 0xea, 0xc, 0xd5);

#undef INTERFACE
#define INTERFACE ID3DXEffect

DECLARE_INTERFACE_(ID3DXEffect, ID3DXBaseEffect)
{
    // ID3DXBaseEffect
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // Descs
    STDMETHOD(GetDesc)(THIS_ D3DXEFFECT_DESC* pDesc) PURE;
    STDMETHOD(GetParameterDesc)(THIS_ D3DXHANDLE hParameter, D3DXPARAMETER_DESC* pDesc) PURE;
    STDMETHOD(GetTechniqueDesc)(THIS_ D3DXHANDLE hTechnique, D3DXTECHNIQUE_DESC* pDesc) PURE;
    STDMETHOD(GetPassDesc)(THIS_ D3DXHANDLE hPass, D3DXPASS_DESC* pDesc) PURE;
    STDMETHOD(GetFunctionDesc)(THIS_ D3DXHANDLE hShader, D3DXFUNCTION_DESC* pDesc) PURE;

    // Handle operations
    STDMETHOD_(D3DXHANDLE, GetParameter)(THIS_ D3DXHANDLE hParameter, UINT Index) PURE;
    STDMETHOD_(D3DXHANDLE, GetParameterByName)(THIS_ D3DXHANDLE hParameter, LPCSTR pName) PURE;
    STDMETHOD_(D3DXHANDLE, GetParameterBySemantic)(THIS_ D3DXHANDLE hParameter, LPCSTR pSemantic) PURE;
    STDMETHOD_(D3DXHANDLE, GetParameterElement)(THIS_ D3DXHANDLE hParameter, UINT Index) PURE;
    STDMETHOD_(D3DXHANDLE, GetTechnique)(THIS_ UINT Index) PURE;
    STDMETHOD_(D3DXHANDLE, GetTechniqueByName)(THIS_ LPCSTR pName) PURE;
    STDMETHOD_(D3DXHANDLE, GetPass)(THIS_ D3DXHANDLE hTechnique, UINT Index) PURE;
    STDMETHOD_(D3DXHANDLE, GetPassByName)(THIS_ D3DXHANDLE hTechnique, LPCSTR pName) PURE;
    STDMETHOD_(D3DXHANDLE, GetFunction)(THIS_ UINT Index) PURE;
    STDMETHOD_(D3DXHANDLE, GetFunctionByName)(THIS_ LPCSTR pName) PURE;
    STDMETHOD_(D3DXHANDLE, GetAnnotation)(THIS_ D3DXHANDLE hObject, UINT Index) PURE;
    STDMETHOD_(D3DXHANDLE, GetAnnotationByName)(THIS_ D3DXHANDLE hObject, LPCSTR pName) PURE;

    // Get/Set Parameters
    STDMETHOD(SetValue)(THIS_ D3DXHANDLE hParameter, LPCVOID pData, UINT Bytes) PURE;
    STDMETHOD(GetValue)(THIS_ D3DXHANDLE hParameter, LPVOID pData, UINT Bytes) PURE;
    STDMETHOD(SetBool)(THIS_ D3DXHANDLE hParameter, BOOL b) PURE;
    STDMETHOD(GetBool)(THIS_ D3DXHANDLE hParameter, BOOL* pb) PURE;
    STDMETHOD(SetBoolArray)(THIS_ D3DXHANDLE hParameter, CONST BOOL* pb, UINT Count) PURE;
    STDMETHOD(GetBoolArray)(THIS_ D3DXHANDLE hParameter, BOOL* pb, UINT Count) PURE;
    STDMETHOD(SetInt)(THIS_ D3DXHANDLE hParameter, INT n) PURE;
    STDMETHOD(GetInt)(THIS_ D3DXHANDLE hParameter, INT* pn) PURE;
    STDMETHOD(SetIntArray)(THIS_ D3DXHANDLE hParameter, CONST INT* pn, UINT Count) PURE;
    STDMETHOD(GetIntArray)(THIS_ D3DXHANDLE hParameter, INT* pn, UINT Count) PURE;
    STDMETHOD(SetFloat)(THIS_ D3DXHANDLE hParameter, FLOAT f) PURE;
    STDMETHOD(GetFloat)(THIS_ D3DXHANDLE hParameter, FLOAT* pf) PURE;
    STDMETHOD(SetFloatArray)(THIS_ D3DXHANDLE hParameter, CONST FLOAT* pf, UINT Count) PURE;
    STDMETHOD(GetFloatArray)(THIS_ D3DXHANDLE hParameter, FLOAT* pf, UINT Count) PURE;
    STDMETHOD(SetVector)(THIS_ D3DXHANDLE hParameter, CONST D3DXVECTOR4* pVector) PURE;
    STDMETHOD(GetVector)(THIS_ D3DXHANDLE hParameter, D3DXVECTOR4* pVector) PURE;
    STDMETHOD(SetVectorArray)(THIS_ D3DXHANDLE hParameter, CONST D3DXVECTOR4* pVector, UINT Count) PURE;
    STDMETHOD(GetVectorArray)(THIS_ D3DXHANDLE hParameter, D3DXVECTOR4* pVector, UINT Count) PURE;
    STDMETHOD(SetMatrix)(THIS_ D3DXHANDLE hParameter, CONST D3DXMATRIX* pMatrix) PURE;
    STDMETHOD(GetMatrix)(THIS_ D3DXHANDLE hParameter, D3DXMATRIX* pMatrix) PURE;
    STDMETHOD(SetMatrixArray)(THIS_ D3DXHANDLE hParameter, CONST D3DXMATRIX* pMatrix, UINT Count) PURE;
    STDMETHOD(GetMatrixArray)(THIS_ D3DXHANDLE hParameter, D3DXMATRIX* pMatrix, UINT Count) PURE;
    STDMETHOD(SetMatrixPointerArray)(THIS_ D3DXHANDLE hParameter, CONST D3DXMATRIX** ppMatrix, UINT Count) PURE;
    STDMETHOD(GetMatrixPointerArray)(THIS_ D3DXHANDLE hParameter, D3DXMATRIX** ppMatrix, UINT Count) PURE;
    STDMETHOD(SetMatrixTranspose)(THIS_ D3DXHANDLE hParameter, CONST D3DXMATRIX* pMatrix) PURE;
    STDMETHOD(GetMatrixTranspose)(THIS_ D3DXHANDLE hParameter, D3DXMATRIX* pMatrix) PURE;
    STDMETHOD(SetMatrixTransposeArray)(THIS_ D3DXHANDLE hParameter, CONST D3DXMATRIX* pMatrix, UINT Count) PURE;
    STDMETHOD(GetMatrixTransposeArray)(THIS_ D3DXHANDLE hParameter, D3DXMATRIX* pMatrix, UINT Count) PURE;
    STDMETHOD(SetMatrixTransposePointerArray)(THIS_ D3DXHANDLE hParameter, CONST D3DXMATRIX** ppMatrix, UINT Count) PURE;
    STDMETHOD(GetMatrixTransposePointerArray)(THIS_ D3DXHANDLE hParameter, D3DXMATRIX** ppMatrix, UINT Count) PURE;
    STDMETHOD(SetString)(THIS_ D3DXHANDLE hParameter, LPCSTR pString) PURE;
    STDMETHOD(GetString)(THIS_ D3DXHANDLE hParameter, LPCSTR* ppString) PURE;
    STDMETHOD(SetTexture)(THIS_ D3DXHANDLE hParameter, LPDIRECT3DBASETEXTURE9 pTexture) PURE;
    STDMETHOD(GetTexture)(THIS_ D3DXHANDLE hParameter, LPDIRECT3DBASETEXTURE9 *ppTexture) PURE;
    STDMETHOD(GetPixelShader)(THIS_ D3DXHANDLE hParameter, LPDIRECT3DPIXELSHADER9 *ppPShader) PURE;
    STDMETHOD(GetVertexShader)(THIS_ D3DXHANDLE hParameter, LPDIRECT3DVERTEXSHADER9 *ppVShader) PURE;

	//Set Range of an Array to pass to device
	//Usefull for sending only a subrange of an array down to the device
	STDMETHOD(SetArrayRange)(THIS_ D3DXHANDLE hParameter, UINT uStart, UINT uEnd) PURE; 
	// ID3DXBaseEffect
    
    
    // Pool
    STDMETHOD(GetPool)(THIS_ LPD3DXEFFECTPOOL* ppPool) PURE;

    // Selecting and setting a technique
    STDMETHOD(SetTechnique)(THIS_ D3DXHANDLE hTechnique) PURE;
    STDMETHOD_(D3DXHANDLE, GetCurrentTechnique)(THIS) PURE;
    STDMETHOD(ValidateTechnique)(THIS_ D3DXHANDLE hTechnique) PURE;
    STDMETHOD(FindNextValidTechnique)(THIS_ D3DXHANDLE hTechnique, D3DXHANDLE *pTechnique) PURE;
    STDMETHOD_(BOOL, IsParameterUsed)(THIS_ D3DXHANDLE hParameter, D3DXHANDLE hTechnique) PURE;

    // Using current technique
    // Begin           starts active technique
    // BeginPass       begins a pass
    // CommitChanges   updates changes to any set calls in the pass. This should be called before
    //                 any DrawPrimitive call to d3d
    // EndPass         ends a pass
    // End             ends active technique
    STDMETHOD(Begin)(THIS_ UINT *pPasses, DWORD Flags) PURE;
    STDMETHOD(BeginPass)(THIS_ UINT Pass) PURE;
    STDMETHOD(CommitChanges)(THIS) PURE;
    STDMETHOD(EndPass)(THIS) PURE;
    STDMETHOD(End)(THIS) PURE;

    // Managing D3D Device
    STDMETHOD(GetDevice)(THIS_ LPDIRECT3DDEVICE9* ppDevice) PURE;
    STDMETHOD(OnLostDevice)(THIS) PURE;
    STDMETHOD(OnResetDevice)(THIS) PURE;

    // Logging device calls
    STDMETHOD(SetStateManager)(THIS_ LPD3DXEFFECTSTATEMANAGER pManager) PURE;
    STDMETHOD(GetStateManager)(THIS_ LPD3DXEFFECTSTATEMANAGER *ppManager) PURE;

    // Parameter blocks
    STDMETHOD(BeginParameterBlock)(THIS) PURE;
    STDMETHOD_(D3DXHANDLE, EndParameterBlock)(THIS) PURE;
    STDMETHOD(ApplyParameterBlock)(THIS_ D3DXHANDLE hParameterBlock) PURE;
    STDMETHOD(DeleteParameterBlock)(THIS_ D3DXHANDLE hParameterBlock) PURE;

    // Cloning
    STDMETHOD(CloneEffect)(THIS_ LPDIRECT3DDEVICE9 pDevice, LPD3DXEFFECT* ppEffect) PURE;
    
    // Fast path for setting variables directly in ID3DXEffect
    STDMETHOD(SetRawValue)(THIS_ D3DXHANDLE hParameter, LPCVOID pData, UINT ByteOffset, UINT Bytes) PURE;
};


//////////////////////////////////////////////////////////////////////////////
// ID3DXEffectCompiler ///////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef interface ID3DXEffectCompiler ID3DXEffectCompiler;
typedef interface ID3DXEffectCompiler *LPD3DXEFFECTCOMPILER;

// {51B8A949-1A31-47e6-BEA0-4B30DB53F1E0}
DEFINE_GUID(IID_ID3DXEffectCompiler, 
0x51b8a949, 0x1a31, 0x47e6, 0xbe, 0xa0, 0x4b, 0x30, 0xdb, 0x53, 0xf1, 0xe0);


#undef INTERFACE
#define INTERFACE ID3DXEffectCompiler

DECLARE_INTERFACE_(ID3DXEffectCompiler, ID3DXBaseEffect)
{
    // ID3DXBaseEffect
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // Descs
    STDMETHOD(GetDesc)(THIS_ D3DXEFFECT_DESC* pDesc) PURE;
    STDMETHOD(GetParameterDesc)(THIS_ D3DXHANDLE hParameter, D3DXPARAMETER_DESC* pDesc) PURE;
    STDMETHOD(GetTechniqueDesc)(THIS_ D3DXHANDLE hTechnique, D3DXTECHNIQUE_DESC* pDesc) PURE;
    STDMETHOD(GetPassDesc)(THIS_ D3DXHANDLE hPass, D3DXPASS_DESC* pDesc) PURE;
    STDMETHOD(GetFunctionDesc)(THIS_ D3DXHANDLE hShader, D3DXFUNCTION_DESC* pDesc) PURE;

    // Handle operations
    STDMETHOD_(D3DXHANDLE, GetParameter)(THIS_ D3DXHANDLE hParameter, UINT Index) PURE;
    STDMETHOD_(D3DXHANDLE, GetParameterByName)(THIS_ D3DXHANDLE hParameter, LPCSTR pName) PURE;
    STDMETHOD_(D3DXHANDLE, GetParameterBySemantic)(THIS_ D3DXHANDLE hParameter, LPCSTR pSemantic) PURE;
    STDMETHOD_(D3DXHANDLE, GetParameterElement)(THIS_ D3DXHANDLE hParameter, UINT Index) PURE;
    STDMETHOD_(D3DXHANDLE, GetTechnique)(THIS_ UINT Index) PURE;
    STDMETHOD_(D3DXHANDLE, GetTechniqueByName)(THIS_ LPCSTR pName) PURE;
    STDMETHOD_(D3DXHANDLE, GetPass)(THIS_ D3DXHANDLE hTechnique, UINT Index) PURE;
    STDMETHOD_(D3DXHANDLE, GetPassByName)(THIS_ D3DXHANDLE hTechnique, LPCSTR pName) PURE;
    STDMETHOD_(D3DXHANDLE, GetFunction)(THIS_ UINT Index) PURE;
    STDMETHOD_(D3DXHANDLE, GetFunctionByName)(THIS_ LPCSTR pName) PURE;
    STDMETHOD_(D3DXHANDLE, GetAnnotation)(THIS_ D3DXHANDLE hObject, UINT Index) PURE;
    STDMETHOD_(D3DXHANDLE, GetAnnotationByName)(THIS_ D3DXHANDLE hObject, LPCSTR pName) PURE;

    // Get/Set Parameters
    STDMETHOD(SetValue)(THIS_ D3DXHANDLE hParameter, LPCVOID pData, UINT Bytes) PURE;
    STDMETHOD(GetValue)(THIS_ D3DXHANDLE hParameter, LPVOID pData, UINT Bytes) PURE;
    STDMETHOD(SetBool)(THIS_ D3DXHANDLE hParameter, BOOL b) PURE;
    STDMETHOD(GetBool)(THIS_ D3DXHANDLE hParameter, BOOL* pb) PURE;
    STDMETHOD(SetBoolArray)(THIS_ D3DXHANDLE hParameter, CONST BOOL* pb, UINT Count) PURE;
    STDMETHOD(GetBoolArray)(THIS_ D3DXHANDLE hParameter, BOOL* pb, UINT Count) PURE;
    STDMETHOD(SetInt)(THIS_ D3DXHANDLE hParameter, INT n) PURE;
    STDMETHOD(GetInt)(THIS_ D3DXHANDLE hParameter, INT* pn) PURE;
    STDMETHOD(SetIntArray)(THIS_ D3DXHANDLE hParameter, CONST INT* pn, UINT Count) PURE;
    STDMETHOD(GetIntArray)(THIS_ D3DXHANDLE hParameter, INT* pn, UINT Count) PURE;
    STDMETHOD(SetFloat)(THIS_ D3DXHANDLE hParameter, FLOAT f) PURE;
    STDMETHOD(GetFloat)(THIS_ D3DXHANDLE hParameter, FLOAT* pf) PURE;
    STDMETHOD(SetFloatArray)(THIS_ D3DXHANDLE hParameter, CONST FLOAT* pf, UINT Count) PURE;
    STDMETHOD(GetFloatArray)(THIS_ D3DXHANDLE hParameter, FLOAT* pf, UINT Count) PURE;
    STDMETHOD(SetVector)(THIS_ D3DXHANDLE hParameter, CONST D3DXVECTOR4* pVector) PURE;
    STDMETHOD(GetVector)(THIS_ D3DXHANDLE hParameter, D3DXVECTOR4* pVector) PURE;
    STDMETHOD(SetVectorArray)(THIS_ D3DXHANDLE hParameter, CONST D3DXVECTOR4* pVector, UINT Count) PURE;
    STDMETHOD(GetVectorArray)(THIS_ D3DXHANDLE hParameter, D3DXVECTOR4* pVector, UINT Count) PURE;
    STDMETHOD(SetMatrix)(THIS_ D3DXHANDLE hParameter, CONST D3DXMATRIX* pMatrix) PURE;
    STDMETHOD(GetMatrix)(THIS_ D3DXHANDLE hParameter, D3DXMATRIX* pMatrix) PURE;
    STDMETHOD(SetMatrixArray)(THIS_ D3DXHANDLE hParameter, CONST D3DXMATRIX* pMatrix, UINT Count) PURE;
    STDMETHOD(GetMatrixArray)(THIS_ D3DXHANDLE hParameter, D3DXMATRIX* pMatrix, UINT Count) PURE;
    STDMETHOD(SetMatrixPointerArray)(THIS_ D3DXHANDLE hParameter, CONST D3DXMATRIX** ppMatrix, UINT Count) PURE;
    STDMETHOD(GetMatrixPointerArray)(THIS_ D3DXHANDLE hParameter, D3DXMATRIX** ppMatrix, UINT Count) PURE;
    STDMETHOD(SetMatrixTranspose)(THIS_ D3DXHANDLE hParameter, CONST D3DXMATRIX* pMatrix) PURE;
    STDMETHOD(GetMatrixTranspose)(THIS_ D3DXHANDLE hParameter, D3DXMATRIX* pMatrix) PURE;
    STDMETHOD(SetMatrixTransposeArray)(THIS_ D3DXHANDLE hParameter, CONST D3DXMATRIX* pMatrix, UINT Count) PURE;
    STDMETHOD(GetMatrixTransposeArray)(THIS_ D3DXHANDLE hParameter, D3DXMATRIX* pMatrix, UINT Count) PURE;
    STDMETHOD(SetMatrixTransposePointerArray)(THIS_ D3DXHANDLE hParameter, CONST D3DXMATRIX** ppMatrix, UINT Count) PURE;
    STDMETHOD(GetMatrixTransposePointerArray)(THIS_ D3DXHANDLE hParameter, D3DXMATRIX** ppMatrix, UINT Count) PURE;
    STDMETHOD(SetString)(THIS_ D3DXHANDLE hParameter, LPCSTR pString) PURE;
    STDMETHOD(GetString)(THIS_ D3DXHANDLE hParameter, LPCSTR* ppString) PURE;
    STDMETHOD(SetTexture)(THIS_ D3DXHANDLE hParameter, LPDIRECT3DBASETEXTURE9 pTexture) PURE;
    STDMETHOD(GetTexture)(THIS_ D3DXHANDLE hParameter, LPDIRECT3DBASETEXTURE9 *ppTexture) PURE;
    STDMETHOD(GetPixelShader)(THIS_ D3DXHANDLE hParameter, LPDIRECT3DPIXELSHADER9 *ppPShader) PURE;
    STDMETHOD(GetVertexShader)(THIS_ D3DXHANDLE hParameter, LPDIRECT3DVERTEXSHADER9 *ppVShader) PURE;
    
	//Set Range of an Array to pass to device
	//Usefull for sending only a subrange of an array down to the device
	STDMETHOD(SetArrayRange)(THIS_ D3DXHANDLE hParameter, UINT uStart, UINT uEnd) PURE; 
	// ID3DXBaseEffect

    // Parameter sharing, specialization, and information
    STDMETHOD(SetLiteral)(THIS_ D3DXHANDLE hParameter, BOOL Literal) PURE;
    STDMETHOD(GetLiteral)(THIS_ D3DXHANDLE hParameter, BOOL *pLiteral) PURE;

    // Compilation
    STDMETHOD(CompileEffect)(THIS_ DWORD Flags,
        LPD3DXBUFFER* ppEffect, LPD3DXBUFFER* ppErrorMsgs) PURE;

    STDMETHOD(CompileShader)(THIS_ D3DXHANDLE hFunction, LPCSTR pTarget, DWORD Flags,
        LPD3DXBUFFER* ppShader, LPD3DXBUFFER* ppErrorMsgs, LPD3DXCONSTANTTABLE* ppConstantTable) PURE;
};


//////////////////////////////////////////////////////////////////////////////
// APIs //////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

//----------------------------------------------------------------------------
// D3DXCreateEffectPool:
// ---------------------
// Creates an effect pool.  Pools are used for sharing parameters between
// multiple effects.  For all effects within a pool, shared parameters of the
// same name all share the same value.
//
// Parameters:
//  ppPool
//      Returns the created pool.
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXCreateEffectPool(
        LPD3DXEFFECTPOOL*               ppPool);


//----------------------------------------------------------------------------
// D3DXCreateEffect:
// -----------------
// Creates an effect from an ascii or binary effect description.
//
// Parameters:
//  pDevice
//      Pointer of the device on which to create the effect
//  pSrcFile
//      Name of the file containing the effect description
//  hSrcModule
//      Module handle. if NULL, current module will be used.
//  pSrcResource
//      Resource name in module
//  pSrcData
//      Pointer to effect description
//  SrcDataSize
//      Size of the effect description in bytes
//  pDefines
//      Optional NULL-terminated array of preprocessor macro definitions.
//  Flags
//      See D3DXSHADER_xxx flags.
//  pSkipConstants
//      A list of semi-colon delimited variable names.  The effect will
//      not set these variables to the device when they are referenced
//      by a shader.  NOTE: the variables specified here must be
//      register bound in the file and must not be used in expressions
//      in passes or samplers or the file will not load.
//  pInclude
//      Optional interface pointer to use for handling #include directives.
//      If this parameter is NULL, #includes will be honored when compiling
//      from file, and will error when compiling from resource or memory.
//  pPool
//      Pointer to ID3DXEffectPool object to use for shared parameters.
//      If NULL, no parameters will be shared.
//  ppEffect
//      Returns a buffer containing created effect.
//  ppCompilationErrors
//      Returns a buffer containing any error messages which occurred during
//      compile.  Or NULL if you do not care about the error messages.
//
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXCreateEffectFromFileA(
        LPDIRECT3DDEVICE9               pDevice,
        LPCSTR                          pSrcFile,
        CONST D3DXMACRO*                pDefines,
        LPD3DXINCLUDE                   pInclude,
        DWORD                           Flags,
        LPD3DXEFFECTPOOL                pPool,
        LPD3DXEFFECT*                   ppEffect,
        LPD3DXBUFFER*                   ppCompilationErrors);

HRESULT WINAPI
    D3DXCreateEffectFromFileW(
        LPDIRECT3DDEVICE9               pDevice,
        LPCWSTR                         pSrcFile,
        CONST D3DXMACRO*                pDefines,
        LPD3DXINCLUDE                   pInclude,
        DWORD                           Flags,
        LPD3DXEFFECTPOOL                pPool,
        LPD3DXEFFECT*                   ppEffect,
        LPD3DXBUFFER*                   ppCompilationErrors);

#ifdef UNICODE
#define D3DXCreateEffectFromFile D3DXCreateEffectFromFileW
#else
#define D3DXCreateEffectFromFile D3DXCreateEffectFromFileA
#endif


HRESULT WINAPI
    D3DXCreateEffectFromResourceA(
        LPDIRECT3DDEVICE9               pDevice,
        HMODULE                         hSrcModule,
        LPCSTR                          pSrcResource,
        CONST D3DXMACRO*                pDefines,
        LPD3DXINCLUDE                   pInclude,
        DWORD                           Flags,
        LPD3DXEFFECTPOOL                pPool,
        LPD3DXEFFECT*                   ppEffect,
        LPD3DXBUFFER*                   ppCompilationErrors);

HRESULT WINAPI
    D3DXCreateEffectFromResourceW(
        LPDIRECT3DDEVICE9               pDevice,
        HMODULE                         hSrcModule,
        LPCWSTR                         pSrcResource,
        CONST D3DXMACRO*                pDefines,
        LPD3DXINCLUDE                   pInclude,
        DWORD                           Flags,
        LPD3DXEFFECTPOOL                pPool,
        LPD3DXEFFECT*                   ppEffect,
        LPD3DXBUFFER*                   ppCompilationErrors);

#ifdef UNICODE
#define D3DXCreateEffectFromResource D3DXCreateEffectFromResourceW
#else
#define D3DXCreateEffectFromResource D3DXCreateEffectFromResourceA
#endif


HRESULT WINAPI
    D3DXCreateEffect(
        LPDIRECT3DDEVICE9               pDevice,
        LPCVOID                         pSrcData,
        UINT                            SrcDataLen,
        CONST D3DXMACRO*                pDefines,
        LPD3DXINCLUDE                   pInclude,
        DWORD                           Flags,
        LPD3DXEFFECTPOOL                pPool,
        LPD3DXEFFECT*                   ppEffect,
        LPD3DXBUFFER*                   ppCompilationErrors);

//
// Ex functions that accept pSkipConstants in addition to other parameters
//

HRESULT WINAPI
    D3DXCreateEffectFromFileExA(
        LPDIRECT3DDEVICE9               pDevice,
        LPCSTR                          pSrcFile,
        CONST D3DXMACRO*                pDefines,
        LPD3DXINCLUDE                   pInclude,
        LPCSTR                          pSkipConstants, 
        DWORD                           Flags,
        LPD3DXEFFECTPOOL                pPool,
        LPD3DXEFFECT*                   ppEffect,
        LPD3DXBUFFER*                   ppCompilationErrors);

HRESULT WINAPI
    D3DXCreateEffectFromFileExW(
        LPDIRECT3DDEVICE9               pDevice,
        LPCWSTR                         pSrcFile,
        CONST D3DXMACRO*                pDefines,
        LPD3DXINCLUDE                   pInclude,
        LPCSTR                          pSkipConstants, 
        DWORD                           Flags,
        LPD3DXEFFECTPOOL                pPool,
        LPD3DXEFFECT*                   ppEffect,
        LPD3DXBUFFER*                   ppCompilationErrors);

#ifdef UNICODE
#define D3DXCreateEffectFromFileEx D3DXCreateEffectFromFileExW
#else
#define D3DXCreateEffectFromFileEx D3DXCreateEffectFromFileExA
#endif


HRESULT WINAPI
    D3DXCreateEffectFromResourceExA(
        LPDIRECT3DDEVICE9               pDevice,
        HMODULE                         hSrcModule,
        LPCSTR                          pSrcResource,
        CONST D3DXMACRO*                pDefines,
        LPD3DXINCLUDE                   pInclude,
        LPCSTR                          pSkipConstants, 
        DWORD                           Flags,
        LPD3DXEFFECTPOOL                pPool,
        LPD3DXEFFECT*                   ppEffect,
        LPD3DXBUFFER*                   ppCompilationErrors);

HRESULT WINAPI
    D3DXCreateEffectFromResourceExW(
        LPDIRECT3DDEVICE9               pDevice,
        HMODULE                         hSrcModule,
        LPCWSTR                         pSrcResource,
        CONST D3DXMACRO*                pDefines,
        LPD3DXINCLUDE                   pInclude,
        LPCSTR                          pSkipConstants, 
        DWORD                           Flags,
        LPD3DXEFFECTPOOL                pPool,
        LPD3DXEFFECT*                   ppEffect,
        LPD3DXBUFFER*                   ppCompilationErrors);

#ifdef UNICODE
#define D3DXCreateEffectFromResourceEx D3DXCreateEffectFromResourceExW
#else
#define D3DXCreateEffectFromResourceEx D3DXCreateEffectFromResourceExA
#endif


HRESULT WINAPI
    D3DXCreateEffectEx(
        LPDIRECT3DDEVICE9               pDevice,
        LPCVOID                         pSrcData,
        UINT                            SrcDataLen,
        CONST D3DXMACRO*                pDefines,
        LPD3DXINCLUDE                   pInclude,
        LPCSTR                          pSkipConstants, 
        DWORD                           Flags,
        LPD3DXEFFECTPOOL                pPool,
        LPD3DXEFFECT*                   ppEffect,
        LPD3DXBUFFER*                   ppCompilationErrors);

//----------------------------------------------------------------------------
// D3DXCreateEffectCompiler:
// -------------------------
// Creates an effect from an ascii or binary effect description.
//
// Parameters:
//  pSrcFile
//      Name of the file containing the effect description
//  hSrcModule
//      Module handle. if NULL, current module will be used.
//  pSrcResource
//      Resource name in module
//  pSrcData
//      Pointer to effect description
//  SrcDataSize
//      Size of the effect description in bytes
//  pDefines
//      Optional NULL-terminated array of preprocessor macro definitions.
//  pInclude
//      Optional interface pointer to use for handling #include directives.
//      If this parameter is NULL, #includes will be honored when compiling
//      from file, and will error when compiling from resource or memory.
//  pPool
//      Pointer to ID3DXEffectPool object to use for shared parameters.
//      If NULL, no parameters will be shared.
//  ppCompiler
//      Returns a buffer containing created effect compiler.
//  ppParseErrors
//      Returns a buffer containing any error messages which occurred during
//      parse.  Or NULL if you do not care about the error messages.
//
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXCreateEffectCompilerFromFileA(
        LPCSTR                          pSrcFile,
        CONST D3DXMACRO*                pDefines,
        LPD3DXINCLUDE                   pInclude,
        DWORD                           Flags,
        LPD3DXEFFECTCOMPILER*           ppCompiler,
        LPD3DXBUFFER*                   ppParseErrors);

HRESULT WINAPI
    D3DXCreateEffectCompilerFromFileW(
        LPCWSTR                         pSrcFile,
        CONST D3DXMACRO*                pDefines,
        LPD3DXINCLUDE                   pInclude,
        DWORD                           Flags,
        LPD3DXEFFECTCOMPILER*           ppCompiler,
        LPD3DXBUFFER*                   ppParseErrors);

#ifdef UNICODE
#define D3DXCreateEffectCompilerFromFile D3DXCreateEffectCompilerFromFileW
#else
#define D3DXCreateEffectCompilerFromFile D3DXCreateEffectCompilerFromFileA
#endif


HRESULT WINAPI
    D3DXCreateEffectCompilerFromResourceA(
        HMODULE                         hSrcModule,
        LPCSTR                          pSrcResource,
        CONST D3DXMACRO*                pDefines,
        LPD3DXINCLUDE                   pInclude,
        DWORD                           Flags,
        LPD3DXEFFECTCOMPILER*           ppCompiler,
        LPD3DXBUFFER*                   ppParseErrors);

HRESULT WINAPI
    D3DXCreateEffectCompilerFromResourceW(
        HMODULE                         hSrcModule,
        LPCWSTR                         pSrcResource,
        CONST D3DXMACRO*                pDefines,
        LPD3DXINCLUDE                   pInclude,
        DWORD                           Flags,
        LPD3DXEFFECTCOMPILER*           ppCompiler,
        LPD3DXBUFFER*                   ppParseErrors);

#ifdef UNICODE
#define D3DXCreateEffectCompilerFromResource D3DXCreateEffectCompilerFromResourceW
#else
#define D3DXCreateEffectCompilerFromResource D3DXCreateEffectCompilerFromResourceA
#endif


HRESULT WINAPI
    D3DXCreateEffectCompiler(
        LPCSTR                          pSrcData,
        UINT                            SrcDataLen,
        CONST D3DXMACRO*                pDefines,
        LPD3DXINCLUDE                   pInclude,
        DWORD                           Flags,
        LPD3DXEFFECTCOMPILER*           ppCompiler,
        LPD3DXBUFFER*                   ppParseErrors);

//----------------------------------------------------------------------------
// D3DXDisassembleEffect:
// -----------------------
//
// Parameters:
//----------------------------------------------------------------------------

HRESULT WINAPI 
    D3DXDisassembleEffect(
        LPD3DXEFFECT pEffect, 
        BOOL EnableColorCode, 
        LPD3DXBUFFER *ppDisassembly);
        


#ifdef __cplusplus
}
#endif //__cplusplus

#endif //__D3DX9EFFECT_H__



//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) Microsoft Corporation.  All Rights Reserved.
//
//  File:       d3dx9tex.h
//  Content:    D3DX texturing APIs
//
//////////////////////////////////////////////////////////////////////////////

#ifndef __D3DX9TEX_H__
#define __D3DX9TEX_H__


//----------------------------------------------------------------------------
// D3DX_FILTER flags:
// ------------------
//
// A valid filter must contain one of these values:
//
//  D3DX_FILTER_NONE
//      No scaling or filtering will take place.  Pixels outside the bounds
//      of the source image are assumed to be transparent black.
//  D3DX_FILTER_POINT
//      Each destination pixel is computed by sampling the nearest pixel
//      from the source image.
//  D3DX_FILTER_LINEAR
//      Each destination pixel is computed by linearly interpolating between
//      the nearest pixels in the source image.  This filter works best 
//      when the scale on each axis is less than 2.
//  D3DX_FILTER_TRIANGLE
//      Every pixel in the source image contributes equally to the
//      destination image.  This is the slowest of all the filters.
//  D3DX_FILTER_BOX
//      Each pixel is computed by averaging a 2x2(x2) box pixels from 
//      the source image. Only works when the dimensions of the 
//      destination are half those of the source. (as with mip maps)
//
// And can be OR'd with any of these optional flags:
//
//  D3DX_FILTER_MIRROR_U
//      Indicates that pixels off the edge of the texture on the U-axis
//      should be mirrored, not wraped.
//  D3DX_FILTER_MIRROR_V
//      Indicates that pixels off the edge of the texture on the V-axis
//      should be mirrored, not wraped.
//  D3DX_FILTER_MIRROR_W
//      Indicates that pixels off the edge of the texture on the W-axis
//      should be mirrored, not wraped.
//  D3DX_FILTER_MIRROR
//      Same as specifying D3DX_FILTER_MIRROR_U | D3DX_FILTER_MIRROR_V |
//      D3DX_FILTER_MIRROR_V
//  D3DX_FILTER_DITHER
//      Dithers the resulting image using a 4x4 order dither pattern.
//  D3DX_FILTER_SRGB_IN
//      Denotes that the input data is in sRGB (gamma 2.2) colorspace.
//  D3DX_FILTER_SRGB_OUT
//      Denotes that the output data is in sRGB (gamma 2.2) colorspace.
//  D3DX_FILTER_SRGB
//      Same as specifying D3DX_FILTER_SRGB_IN | D3DX_FILTER_SRGB_OUT
//
//----------------------------------------------------------------------------

#define D3DX_FILTER_NONE             (1 << 0)
#define D3DX_FILTER_POINT            (2 << 0)
#define D3DX_FILTER_LINEAR           (3 << 0)
#define D3DX_FILTER_TRIANGLE         (4 << 0)
#define D3DX_FILTER_BOX              (5 << 0)

#define D3DX_FILTER_MIRROR_U         (1 << 16)
#define D3DX_FILTER_MIRROR_V         (2 << 16)
#define D3DX_FILTER_MIRROR_W         (4 << 16)
#define D3DX_FILTER_MIRROR           (7 << 16)

#define D3DX_FILTER_DITHER           (1 << 19)
#define D3DX_FILTER_DITHER_DIFFUSION (2 << 19)

#define D3DX_FILTER_SRGB_IN          (1 << 21)
#define D3DX_FILTER_SRGB_OUT         (2 << 21)
#define D3DX_FILTER_SRGB             (3 << 21)


//-----------------------------------------------------------------------------
// D3DX_SKIP_DDS_MIP_LEVELS is used to skip mip levels when loading a DDS file:
//-----------------------------------------------------------------------------

#define D3DX_SKIP_DDS_MIP_LEVELS_MASK   0x1F
#define D3DX_SKIP_DDS_MIP_LEVELS_SHIFT  26
#define D3DX_SKIP_DDS_MIP_LEVELS(levels, filter) ((((levels) & D3DX_SKIP_DDS_MIP_LEVELS_MASK) << D3DX_SKIP_DDS_MIP_LEVELS_SHIFT) | ((filter) == D3DX_DEFAULT ? D3DX_FILTER_BOX : (filter)))




//----------------------------------------------------------------------------
// D3DX_NORMALMAP flags:
// ---------------------
// These flags are used to control how D3DXComputeNormalMap generates normal
// maps.  Any number of these flags may be OR'd together in any combination.
//
//  D3DX_NORMALMAP_MIRROR_U
//      Indicates that pixels off the edge of the texture on the U-axis
//      should be mirrored, not wraped.
//  D3DX_NORMALMAP_MIRROR_V
//      Indicates that pixels off the edge of the texture on the V-axis
//      should be mirrored, not wraped.
//  D3DX_NORMALMAP_MIRROR
//      Same as specifying D3DX_NORMALMAP_MIRROR_U | D3DX_NORMALMAP_MIRROR_V
//  D3DX_NORMALMAP_INVERTSIGN
//      Inverts the direction of each normal 
//  D3DX_NORMALMAP_COMPUTE_OCCLUSION
//      Compute the per pixel Occlusion term and encodes it into the alpha.
//      An Alpha of 1 means that the pixel is not obscured in anyway, and
//      an alpha of 0 would mean that the pixel is completly obscured.
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

#define D3DX_NORMALMAP_MIRROR_U     (1 << 16)
#define D3DX_NORMALMAP_MIRROR_V     (2 << 16)
#define D3DX_NORMALMAP_MIRROR       (3 << 16)
#define D3DX_NORMALMAP_INVERTSIGN   (8 << 16)
#define D3DX_NORMALMAP_COMPUTE_OCCLUSION (16 << 16)




//----------------------------------------------------------------------------
// D3DX_CHANNEL flags:
// -------------------
// These flags are used by functions which operate on or more channels
// in a texture.
//
// D3DX_CHANNEL_RED
//     Indicates the red channel should be used
// D3DX_CHANNEL_BLUE
//     Indicates the blue channel should be used
// D3DX_CHANNEL_GREEN
//     Indicates the green channel should be used
// D3DX_CHANNEL_ALPHA
//     Indicates the alpha channel should be used
// D3DX_CHANNEL_LUMINANCE
//     Indicates the luminaces of the red green and blue channels should be 
//     used.
//
//----------------------------------------------------------------------------

#define D3DX_CHANNEL_RED            (1 << 0)
#define D3DX_CHANNEL_BLUE           (1 << 1)
#define D3DX_CHANNEL_GREEN          (1 << 2)
#define D3DX_CHANNEL_ALPHA          (1 << 3)
#define D3DX_CHANNEL_LUMINANCE      (1 << 4)




//----------------------------------------------------------------------------
// D3DXIMAGE_FILEFORMAT:
// ---------------------
// This enum is used to describe supported image file formats.
//
//----------------------------------------------------------------------------

typedef enum _D3DXIMAGE_FILEFORMAT
{
    D3DXIFF_BMP         = 0,
    D3DXIFF_JPG         = 1,
    D3DXIFF_TGA         = 2,
    D3DXIFF_PNG         = 3,
    D3DXIFF_DDS         = 4,
    D3DXIFF_PPM         = 5,
    D3DXIFF_DIB         = 6,
    D3DXIFF_HDR         = 7,       //high dynamic range formats
    D3DXIFF_PFM         = 8,       //
    D3DXIFF_FORCE_DWORD = 0x7fffffff

} D3DXIMAGE_FILEFORMAT;


//----------------------------------------------------------------------------
// LPD3DXFILL2D and LPD3DXFILL3D:
// ------------------------------
// Function types used by the texture fill functions.
//
// Parameters:
//  pOut
//      Pointer to a vector which the function uses to return its result.
//      X,Y,Z,W will be mapped to R,G,B,A respectivly. 
//  pTexCoord
//      Pointer to a vector containing the coordinates of the texel currently 
//      being evaluated.  Textures and VolumeTexture texcoord components 
//      range from 0 to 1. CubeTexture texcoord component range from -1 to 1.
//  pTexelSize
//      Pointer to a vector containing the dimensions of the current texel.
//  pData
//      Pointer to user data.
//
//----------------------------------------------------------------------------

typedef VOID (WINAPI *LPD3DXFILL2D)(D3DXVECTOR4 *pOut, 
    CONST D3DXVECTOR2 *pTexCoord, CONST D3DXVECTOR2 *pTexelSize, LPVOID pData);

typedef VOID (WINAPI *LPD3DXFILL3D)(D3DXVECTOR4 *pOut, 
    CONST D3DXVECTOR3 *pTexCoord, CONST D3DXVECTOR3 *pTexelSize, LPVOID pData);
 


//----------------------------------------------------------------------------
// D3DXIMAGE_INFO:
// ---------------
// This structure is used to return a rough description of what the
// the original contents of an image file looked like.
// 
//  Width
//      Width of original image in pixels
//  Height
//      Height of original image in pixels
//  Depth
//      Depth of original image in pixels
//  MipLevels
//      Number of mip levels in original image
//  Format
//      D3D format which most closely describes the data in original image
//  ResourceType
//      D3DRESOURCETYPE representing the type of texture stored in the file.
//      D3DRTYPE_TEXTURE, D3DRTYPE_VOLUMETEXTURE, or D3DRTYPE_CUBETEXTURE.
//  ImageFileFormat
//      D3DXIMAGE_FILEFORMAT representing the format of the image file.
//
//----------------------------------------------------------------------------

typedef struct _D3DXIMAGE_INFO
{
    UINT                    Width;
    UINT                    Height;
    UINT                    Depth;
    UINT                    MipLevels;
    D3DFORMAT               Format;
    D3DRESOURCETYPE         ResourceType;
    D3DXIMAGE_FILEFORMAT    ImageFileFormat;

} D3DXIMAGE_INFO;





#ifdef __cplusplus
extern "C" {
#endif //__cplusplus



//////////////////////////////////////////////////////////////////////////////
// Image File APIs ///////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
;
//----------------------------------------------------------------------------
// GetImageInfoFromFile/Resource:
// ------------------------------
// Fills in a D3DXIMAGE_INFO struct with information about an image file.
//
// Parameters:
//  pSrcFile
//      File name of the source image.
//  pSrcModule
//      Module where resource is located, or NULL for module associated
//      with image the os used to create the current process.
//  pSrcResource
//      Resource name
//  pSrcData
//      Pointer to file in memory.
//  SrcDataSize
//      Size in bytes of file in memory.
//  pSrcInfo
//      Pointer to a D3DXIMAGE_INFO structure to be filled in with the 
//      description of the data in the source image file.
//
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXGetImageInfoFromFileA(
        LPCSTR                    pSrcFile,
        D3DXIMAGE_INFO*           pSrcInfo);

HRESULT WINAPI
    D3DXGetImageInfoFromFileW(
        LPCWSTR                   pSrcFile,
        D3DXIMAGE_INFO*           pSrcInfo);

#ifdef UNICODE
#define D3DXGetImageInfoFromFile D3DXGetImageInfoFromFileW
#else
#define D3DXGetImageInfoFromFile D3DXGetImageInfoFromFileA
#endif


HRESULT WINAPI
    D3DXGetImageInfoFromResourceA(
        HMODULE                   hSrcModule,
        LPCSTR                    pSrcResource,
        D3DXIMAGE_INFO*           pSrcInfo);

HRESULT WINAPI
    D3DXGetImageInfoFromResourceW(
        HMODULE                   hSrcModule,
        LPCWSTR                   pSrcResource,
        D3DXIMAGE_INFO*           pSrcInfo);

#ifdef UNICODE
#define D3DXGetImageInfoFromResource D3DXGetImageInfoFromResourceW
#else
#define D3DXGetImageInfoFromResource D3DXGetImageInfoFromResourceA
#endif


HRESULT WINAPI
    D3DXGetImageInfoFromFileInMemory(
        LPCVOID                   pSrcData,
        UINT                      SrcDataSize,
        D3DXIMAGE_INFO*           pSrcInfo);




//////////////////////////////////////////////////////////////////////////////
// Load/Save Surface APIs ////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// D3DXLoadSurfaceFromFile/Resource:
// ---------------------------------
// Load surface from a file or resource
//
// Parameters:
//  pDestSurface
//      Destination surface, which will receive the image.
//  pDestPalette
//      Destination palette of 256 colors, or NULL
//  pDestRect
//      Destination rectangle, or NULL for entire surface
//  pSrcFile
//      File name of the source image.
//  pSrcModule
//      Module where resource is located, or NULL for module associated
//      with image the os used to create the current process.
//  pSrcResource
//      Resource name
//  pSrcData
//      Pointer to file in memory.
//  SrcDataSize
//      Size in bytes of file in memory.
//  pSrcRect
//      Source rectangle, or NULL for entire image
//  Filter
//      D3DX_FILTER flags controlling how the image is filtered.
//      Or D3DX_DEFAULT for D3DX_FILTER_TRIANGLE.
//  ColorKey
//      Color to replace with transparent black, or 0 to disable colorkey.
//      This is always a 32-bit ARGB color, independent of the source image
//      format.  Alpha is significant, and should usually be set to FF for 
//      opaque colorkeys.  (ex. Opaque black == 0xff000000)
//  pSrcInfo
//      Pointer to a D3DXIMAGE_INFO structure to be filled in with the 
//      description of the data in the source image file, or NULL.
//
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXLoadSurfaceFromFileA(
        LPDIRECT3DSURFACE9        pDestSurface,
        CONST PALETTEENTRY*       pDestPalette,
        CONST RECT*               pDestRect,
        LPCSTR                    pSrcFile,
        CONST RECT*               pSrcRect,
        DWORD                     Filter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo);

HRESULT WINAPI
    D3DXLoadSurfaceFromFileW(
        LPDIRECT3DSURFACE9        pDestSurface,
        CONST PALETTEENTRY*       pDestPalette,
        CONST RECT*               pDestRect,
        LPCWSTR                   pSrcFile,
        CONST RECT*               pSrcRect,
        DWORD                     Filter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo);

#ifdef UNICODE
#define D3DXLoadSurfaceFromFile D3DXLoadSurfaceFromFileW
#else
#define D3DXLoadSurfaceFromFile D3DXLoadSurfaceFromFileA
#endif



HRESULT WINAPI
    D3DXLoadSurfaceFromResourceA(
        LPDIRECT3DSURFACE9        pDestSurface,
        CONST PALETTEENTRY*       pDestPalette,
        CONST RECT*               pDestRect,
        HMODULE                   hSrcModule,
        LPCSTR                    pSrcResource,
        CONST RECT*               pSrcRect,
        DWORD                     Filter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo);

HRESULT WINAPI
    D3DXLoadSurfaceFromResourceW(
        LPDIRECT3DSURFACE9        pDestSurface,
        CONST PALETTEENTRY*       pDestPalette,
        CONST RECT*               pDestRect,
        HMODULE                   hSrcModule,
        LPCWSTR                   pSrcResource,
        CONST RECT*               pSrcRect,
        DWORD                     Filter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo);


#ifdef UNICODE
#define D3DXLoadSurfaceFromResource D3DXLoadSurfaceFromResourceW
#else
#define D3DXLoadSurfaceFromResource D3DXLoadSurfaceFromResourceA
#endif



HRESULT WINAPI
    D3DXLoadSurfaceFromFileInMemory(
        LPDIRECT3DSURFACE9        pDestSurface,
        CONST PALETTEENTRY*       pDestPalette,
        CONST RECT*               pDestRect,
        LPCVOID                   pSrcData,
        UINT                      SrcDataSize,
        CONST RECT*               pSrcRect,
        DWORD                     Filter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo);



//----------------------------------------------------------------------------
// D3DXLoadSurfaceFromSurface:
// ---------------------------
// Load surface from another surface (with color conversion)
//
// Parameters:
//  pDestSurface
//      Destination surface, which will receive the image.
//  pDestPalette
//      Destination palette of 256 colors, or NULL
//  pDestRect
//      Destination rectangle, or NULL for entire surface
//  pSrcSurface
//      Source surface
//  pSrcPalette
//      Source palette of 256 colors, or NULL
//  pSrcRect
//      Source rectangle, or NULL for entire surface
//  Filter
//      D3DX_FILTER flags controlling how the image is filtered.
//      Or D3DX_DEFAULT for D3DX_FILTER_TRIANGLE.
//  ColorKey
//      Color to replace with transparent black, or 0 to disable colorkey.
//      This is always a 32-bit ARGB color, independent of the source image
//      format.  Alpha is significant, and should usually be set to FF for 
//      opaque colorkeys.  (ex. Opaque black == 0xff000000)
//
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXLoadSurfaceFromSurface(
        LPDIRECT3DSURFACE9        pDestSurface,
        CONST PALETTEENTRY*       pDestPalette,
        CONST RECT*               pDestRect,
        LPDIRECT3DSURFACE9        pSrcSurface,
        CONST PALETTEENTRY*       pSrcPalette,
        CONST RECT*               pSrcRect,
        DWORD                     Filter,
        D3DCOLOR                  ColorKey);


//----------------------------------------------------------------------------
// D3DXLoadSurfaceFromMemory:
// --------------------------
// Load surface from memory.
//
// Parameters:
//  pDestSurface
//      Destination surface, which will receive the image.
//  pDestPalette
//      Destination palette of 256 colors, or NULL
//  pDestRect
//      Destination rectangle, or NULL for entire surface
//  pSrcMemory
//      Pointer to the top-left corner of the source image in memory
//  SrcFormat
//      Pixel format of the source image.
//  SrcPitch
//      Pitch of source image, in bytes.  For DXT formats, this number
//      should represent the width of one row of cells, in bytes.
//  pSrcPalette
//      Source palette of 256 colors, or NULL
//  pSrcRect
//      Source rectangle.
//  Filter
//      D3DX_FILTER flags controlling how the image is filtered.
//      Or D3DX_DEFAULT for D3DX_FILTER_TRIANGLE.
//  ColorKey
//      Color to replace with transparent black, or 0 to disable colorkey.
//      This is always a 32-bit ARGB color, independent of the source image
//      format.  Alpha is significant, and should usually be set to FF for 
//      opaque colorkeys.  (ex. Opaque black == 0xff000000)
//
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXLoadSurfaceFromMemory(
        LPDIRECT3DSURFACE9        pDestSurface,
        CONST PALETTEENTRY*       pDestPalette,
        CONST RECT*               pDestRect,
        LPCVOID                   pSrcMemory,
        D3DFORMAT                 SrcFormat,
        UINT                      SrcPitch,
        CONST PALETTEENTRY*       pSrcPalette,
        CONST RECT*               pSrcRect,
        DWORD                     Filter,
        D3DCOLOR                  ColorKey);


//----------------------------------------------------------------------------
// D3DXSaveSurfaceToFile:
// ----------------------
// Save a surface to a image file.
//
// Parameters:
//  pDestFile
//      File name of the destination file
//  DestFormat
//      D3DXIMAGE_FILEFORMAT specifying file format to use when saving.
//  pSrcSurface
//      Source surface, containing the image to be saved
//  pSrcPalette
//      Source palette of 256 colors, or NULL
//  pSrcRect
//      Source rectangle, or NULL for the entire image
//
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXSaveSurfaceToFileA(
        LPCSTR                    pDestFile,
        D3DXIMAGE_FILEFORMAT      DestFormat,
        LPDIRECT3DSURFACE9        pSrcSurface,
        CONST PALETTEENTRY*       pSrcPalette,
        CONST RECT*               pSrcRect);

HRESULT WINAPI
    D3DXSaveSurfaceToFileW(
        LPCWSTR                   pDestFile,
        D3DXIMAGE_FILEFORMAT      DestFormat,
        LPDIRECT3DSURFACE9        pSrcSurface,
        CONST PALETTEENTRY*       pSrcPalette,
        CONST RECT*               pSrcRect);

#ifdef UNICODE
#define D3DXSaveSurfaceToFile D3DXSaveSurfaceToFileW
#else
#define D3DXSaveSurfaceToFile D3DXSaveSurfaceToFileA
#endif

//----------------------------------------------------------------------------
// D3DXSaveSurfaceToFileInMemory:
// ----------------------
// Save a surface to a image file.
//
// Parameters:
//  ppDestBuf
//      address of pointer to d3dxbuffer for returning data bits
//  DestFormat
//      D3DXIMAGE_FILEFORMAT specifying file format to use when saving.
//  pSrcSurface
//      Source surface, containing the image to be saved
//  pSrcPalette
//      Source palette of 256 colors, or NULL
//  pSrcRect
//      Source rectangle, or NULL for the entire image
//
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXSaveSurfaceToFileInMemory(
        LPD3DXBUFFER*             ppDestBuf,
        D3DXIMAGE_FILEFORMAT      DestFormat,
        LPDIRECT3DSURFACE9        pSrcSurface,
        CONST PALETTEENTRY*       pSrcPalette,
        CONST RECT*               pSrcRect);


//////////////////////////////////////////////////////////////////////////////
// Load/Save Volume APIs /////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// D3DXLoadVolumeFromFile/Resource:
// --------------------------------
// Load volume from a file or resource
//
// Parameters:
//  pDestVolume
//      Destination volume, which will receive the image.
//  pDestPalette
//      Destination palette of 256 colors, or NULL
//  pDestBox
//      Destination box, or NULL for entire volume
//  pSrcFile
//      File name of the source image.
//  pSrcModule
//      Module where resource is located, or NULL for module associated
//      with image the os used to create the current process.
//  pSrcResource
//      Resource name
//  pSrcData
//      Pointer to file in memory.
//  SrcDataSize
//      Size in bytes of file in memory.
//  pSrcBox
//      Source box, or NULL for entire image
//  Filter
//      D3DX_FILTER flags controlling how the image is filtered.
//      Or D3DX_DEFAULT for D3DX_FILTER_TRIANGLE.
//  ColorKey
//      Color to replace with transparent black, or 0 to disable colorkey.
//      This is always a 32-bit ARGB color, independent of the source image
//      format.  Alpha is significant, and should usually be set to FF for 
//      opaque colorkeys.  (ex. Opaque black == 0xff000000)
//  pSrcInfo
//      Pointer to a D3DXIMAGE_INFO structure to be filled in with the 
//      description of the data in the source image file, or NULL.
//
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXLoadVolumeFromFileA(
        LPDIRECT3DVOLUME9         pDestVolume,
        CONST PALETTEENTRY*       pDestPalette,
        CONST D3DBOX*             pDestBox,
        LPCSTR                    pSrcFile,
        CONST D3DBOX*             pSrcBox,
        DWORD                     Filter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo);

HRESULT WINAPI
    D3DXLoadVolumeFromFileW(
        LPDIRECT3DVOLUME9         pDestVolume,
        CONST PALETTEENTRY*       pDestPalette,
        CONST D3DBOX*             pDestBox,
        LPCWSTR                   pSrcFile,
        CONST D3DBOX*             pSrcBox,
        DWORD                     Filter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo);

#ifdef UNICODE
#define D3DXLoadVolumeFromFile D3DXLoadVolumeFromFileW
#else
#define D3DXLoadVolumeFromFile D3DXLoadVolumeFromFileA
#endif


HRESULT WINAPI
    D3DXLoadVolumeFromResourceA(
        LPDIRECT3DVOLUME9         pDestVolume,
        CONST PALETTEENTRY*       pDestPalette,
        CONST D3DBOX*             pDestBox,
        HMODULE                   hSrcModule,
        LPCSTR                    pSrcResource,
        CONST D3DBOX*             pSrcBox,
        DWORD                     Filter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo);

HRESULT WINAPI
    D3DXLoadVolumeFromResourceW(
        LPDIRECT3DVOLUME9         pDestVolume,
        CONST PALETTEENTRY*       pDestPalette,
        CONST D3DBOX*             pDestBox,
        HMODULE                   hSrcModule,
        LPCWSTR                   pSrcResource,
        CONST D3DBOX*             pSrcBox,
        DWORD                     Filter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo);

#ifdef UNICODE
#define D3DXLoadVolumeFromResource D3DXLoadVolumeFromResourceW
#else
#define D3DXLoadVolumeFromResource D3DXLoadVolumeFromResourceA
#endif



HRESULT WINAPI
    D3DXLoadVolumeFromFileInMemory(
        LPDIRECT3DVOLUME9         pDestVolume,
        CONST PALETTEENTRY*       pDestPalette,
        CONST D3DBOX*             pDestBox,
        LPCVOID                   pSrcData,
        UINT                      SrcDataSize,
        CONST D3DBOX*             pSrcBox,
        DWORD                     Filter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo);



//----------------------------------------------------------------------------
// D3DXLoadVolumeFromVolume:
// -------------------------
// Load volume from another volume (with color conversion)
//
// Parameters:
//  pDestVolume
//      Destination volume, which will receive the image.
//  pDestPalette
//      Destination palette of 256 colors, or NULL
//  pDestBox
//      Destination box, or NULL for entire volume
//  pSrcVolume
//      Source volume
//  pSrcPalette
//      Source palette of 256 colors, or NULL
//  pSrcBox
//      Source box, or NULL for entire volume
//  Filter
//      D3DX_FILTER flags controlling how the image is filtered.
//      Or D3DX_DEFAULT for D3DX_FILTER_TRIANGLE.
//  ColorKey
//      Color to replace with transparent black, or 0 to disable colorkey.
//      This is always a 32-bit ARGB color, independent of the source image
//      format.  Alpha is significant, and should usually be set to FF for 
//      opaque colorkeys.  (ex. Opaque black == 0xff000000)
//
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXLoadVolumeFromVolume(
        LPDIRECT3DVOLUME9         pDestVolume,
        CONST PALETTEENTRY*       pDestPalette,
        CONST D3DBOX*             pDestBox,
        LPDIRECT3DVOLUME9         pSrcVolume,
        CONST PALETTEENTRY*       pSrcPalette,
        CONST D3DBOX*             pSrcBox,
        DWORD                     Filter,
        D3DCOLOR                  ColorKey);



//----------------------------------------------------------------------------
// D3DXLoadVolumeFromMemory:
// -------------------------
// Load volume from memory.
//
// Parameters:
//  pDestVolume
//      Destination volume, which will receive the image.
//  pDestPalette
//      Destination palette of 256 colors, or NULL
//  pDestBox
//      Destination box, or NULL for entire volume
//  pSrcMemory
//      Pointer to the top-left corner of the source volume in memory
//  SrcFormat
//      Pixel format of the source volume.
//  SrcRowPitch
//      Pitch of source image, in bytes.  For DXT formats, this number
//      should represent the size of one row of cells, in bytes.
//  SrcSlicePitch
//      Pitch of source image, in bytes.  For DXT formats, this number
//      should represent the size of one slice of cells, in bytes.
//  pSrcPalette
//      Source palette of 256 colors, or NULL
//  pSrcBox
//      Source box.
//  Filter
//      D3DX_FILTER flags controlling how the image is filtered.
//      Or D3DX_DEFAULT for D3DX_FILTER_TRIANGLE.
//  ColorKey
//      Color to replace with transparent black, or 0 to disable colorkey.
//      This is always a 32-bit ARGB color, independent of the source image
//      format.  Alpha is significant, and should usually be set to FF for 
//      opaque colorkeys.  (ex. Opaque black == 0xff000000)
//
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXLoadVolumeFromMemory(
        LPDIRECT3DVOLUME9         pDestVolume,
        CONST PALETTEENTRY*       pDestPalette,
        CONST D3DBOX*             pDestBox,
        LPCVOID                   pSrcMemory,
        D3DFORMAT                 SrcFormat,
        UINT                      SrcRowPitch,
        UINT                      SrcSlicePitch,
        CONST PALETTEENTRY*       pSrcPalette,
        CONST D3DBOX*             pSrcBox,
        DWORD                     Filter,
        D3DCOLOR                  ColorKey);



//----------------------------------------------------------------------------
// D3DXSaveVolumeToFile:
// ---------------------
// Save a volume to a image file.
//
// Parameters:
//  pDestFile
//      File name of the destination file
//  DestFormat
//      D3DXIMAGE_FILEFORMAT specifying file format to use when saving.
//  pSrcVolume
//      Source volume, containing the image to be saved
//  pSrcPalette
//      Source palette of 256 colors, or NULL
//  pSrcBox
//      Source box, or NULL for the entire volume
//
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXSaveVolumeToFileA(
        LPCSTR                    pDestFile,
        D3DXIMAGE_FILEFORMAT      DestFormat,
        LPDIRECT3DVOLUME9         pSrcVolume,
        CONST PALETTEENTRY*       pSrcPalette,
        CONST D3DBOX*             pSrcBox);

HRESULT WINAPI
    D3DXSaveVolumeToFileW(
        LPCWSTR                   pDestFile,
        D3DXIMAGE_FILEFORMAT      DestFormat,
        LPDIRECT3DVOLUME9         pSrcVolume,
        CONST PALETTEENTRY*       pSrcPalette,
        CONST D3DBOX*             pSrcBox);

#ifdef UNICODE
#define D3DXSaveVolumeToFile D3DXSaveVolumeToFileW
#else
#define D3DXSaveVolumeToFile D3DXSaveVolumeToFileA
#endif


//----------------------------------------------------------------------------
// D3DXSaveVolumeToFileInMemory:
// ---------------------
// Save a volume to a image file.
//
// Parameters:
//  pDestFile
//      File name of the destination file
//  DestFormat
//      D3DXIMAGE_FILEFORMAT specifying file format to use when saving.
//  pSrcVolume
//      Source volume, containing the image to be saved
//  pSrcPalette
//      Source palette of 256 colors, or NULL
//  pSrcBox
//      Source box, or NULL for the entire volume
//
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXSaveVolumeToFileInMemory(
        LPD3DXBUFFER*             ppDestBuf,
        D3DXIMAGE_FILEFORMAT      DestFormat,
        LPDIRECT3DVOLUME9         pSrcVolume,
        CONST PALETTEENTRY*       pSrcPalette,
        CONST D3DBOX*             pSrcBox);

//////////////////////////////////////////////////////////////////////////////
// Create/Save Texture APIs //////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// D3DXCheckTextureRequirements:
// -----------------------------
// Checks texture creation parameters.  If parameters are invalid, this
// function returns corrected parameters.
//
// Parameters:
//
//  pDevice
//      The D3D device to be used
//  pWidth, pHeight, pDepth, pSize
//      Desired size in pixels, or NULL.  Returns corrected size.
//  pNumMipLevels
//      Number of desired mipmap levels, or NULL.  Returns corrected number.
//  Usage
//      Texture usage flags
//  pFormat
//      Desired pixel format, or NULL.  Returns corrected format.
//  Pool
//      Memory pool to be used to create texture
//
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXCheckTextureRequirements(
        LPDIRECT3DDEVICE9         pDevice,
        UINT*                     pWidth,
        UINT*                     pHeight,
        UINT*                     pNumMipLevels,
        DWORD                     Usage,
        D3DFORMAT*                pFormat,
        D3DPOOL                   Pool);

HRESULT WINAPI
    D3DXCheckCubeTextureRequirements(
        LPDIRECT3DDEVICE9         pDevice,
        UINT*                     pSize,
        UINT*                     pNumMipLevels,
        DWORD                     Usage,
        D3DFORMAT*                pFormat,
        D3DPOOL                   Pool);

HRESULT WINAPI
    D3DXCheckVolumeTextureRequirements(
        LPDIRECT3DDEVICE9         pDevice,
        UINT*                     pWidth,
        UINT*                     pHeight,
        UINT*                     pDepth,
        UINT*                     pNumMipLevels,
        DWORD                     Usage,
        D3DFORMAT*                pFormat,
        D3DPOOL                   Pool);


//----------------------------------------------------------------------------
// D3DXCreateTexture:
// ------------------
// Create an empty texture
//
// Parameters:
//
//  pDevice
//      The D3D device with which the texture is going to be used.
//  Width, Height, Depth, Size
//      size in pixels. these must be non-zero
//  MipLevels
//      number of mip levels desired. if zero or D3DX_DEFAULT, a complete
//      mipmap chain will be created.
//  Usage
//      Texture usage flags
//  Format
//      Pixel format.
//  Pool
//      Memory pool to be used to create texture
//  ppTexture, ppCubeTexture, ppVolumeTexture
//      The texture object that will be created
//
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXCreateTexture(
        LPDIRECT3DDEVICE9         pDevice,
        UINT                      Width,
        UINT                      Height,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        LPDIRECT3DTEXTURE9*       ppTexture);

HRESULT WINAPI
    D3DXCreateCubeTexture(
        LPDIRECT3DDEVICE9         pDevice,
        UINT                      Size,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        LPDIRECT3DCUBETEXTURE9*   ppCubeTexture);

HRESULT WINAPI
    D3DXCreateVolumeTexture(
        LPDIRECT3DDEVICE9         pDevice,
        UINT                      Width,
        UINT                      Height,
        UINT                      Depth,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        LPDIRECT3DVOLUMETEXTURE9* ppVolumeTexture);



//----------------------------------------------------------------------------
// D3DXCreateTextureFromFile/Resource:
// -----------------------------------
// Create a texture object from a file or resource.
//
// Parameters:
//
//  pDevice
//      The D3D device with which the texture is going to be used.
//  pSrcFile
//      File name.
//  hSrcModule
//      Module handle. if NULL, current module will be used.
//  pSrcResource
//      Resource name in module
//  pvSrcData
//      Pointer to file in memory.
//  SrcDataSize
//      Size in bytes of file in memory.
//  Width, Height, Depth, Size
//      Size in pixels.  If zero or D3DX_DEFAULT, the size will be taken from 
//      the file and rounded up to a power of two.  If D3DX_DEFAULT_NONPOW2, 
//      and the device supports NONPOW2 textures, the size will not be rounded.
//      If D3DX_FROM_FILE, the size will be taken exactly as it is in the file, 
//      and the call will fail if this violates device capabilities.
//  MipLevels
//      Number of mip levels.  If zero or D3DX_DEFAULT, a complete mipmap
//      chain will be created.  If D3DX_FROM_FILE, the size will be taken 
//      exactly as it is in the file, and the call will fail if this violates 
//      device capabilities.
//  Usage
//      Texture usage flags
//  Format
//      Desired pixel format.  If D3DFMT_UNKNOWN, the format will be
//      taken from the file.  If D3DFMT_FROM_FILE, the format will be taken
//      exactly as it is in the file, and the call will fail if the device does
//      not support the given format.
//  Pool
//      Memory pool to be used to create texture
//  Filter
//      D3DX_FILTER flags controlling how the image is filtered.
//      Or D3DX_DEFAULT for D3DX_FILTER_TRIANGLE.
//  MipFilter
//      D3DX_FILTER flags controlling how each miplevel is filtered.
//      Or D3DX_DEFAULT for D3DX_FILTER_BOX.
//      Use the D3DX_SKIP_DDS_MIP_LEVELS macro to specify both a filter and the
//      number of mip levels to skip when loading DDS files.
//  ColorKey
//      Color to replace with transparent black, or 0 to disable colorkey.
//      This is always a 32-bit ARGB color, independent of the source image
//      format.  Alpha is significant, and should usually be set to FF for 
//      opaque colorkeys.  (ex. Opaque black == 0xff000000)
//  pSrcInfo
//      Pointer to a D3DXIMAGE_INFO structure to be filled in with the 
//      description of the data in the source image file, or NULL.
//  pPalette
//      256 color palette to be filled in, or NULL
//  ppTexture, ppCubeTexture, ppVolumeTexture
//      The texture object that will be created
//
//----------------------------------------------------------------------------

// FromFile

HRESULT WINAPI
    D3DXCreateTextureFromFileA(
        LPDIRECT3DDEVICE9         pDevice,
        LPCSTR                    pSrcFile,
        LPDIRECT3DTEXTURE9*       ppTexture);

HRESULT WINAPI
    D3DXCreateTextureFromFileW(
        LPDIRECT3DDEVICE9         pDevice,
        LPCWSTR                   pSrcFile,
        LPDIRECT3DTEXTURE9*       ppTexture);

#ifdef UNICODE
#define D3DXCreateTextureFromFile D3DXCreateTextureFromFileW
#else
#define D3DXCreateTextureFromFile D3DXCreateTextureFromFileA
#endif


HRESULT WINAPI
    D3DXCreateCubeTextureFromFileA(
        LPDIRECT3DDEVICE9         pDevice,
        LPCSTR                    pSrcFile,
        LPDIRECT3DCUBETEXTURE9*   ppCubeTexture);

HRESULT WINAPI
    D3DXCreateCubeTextureFromFileW(
        LPDIRECT3DDEVICE9         pDevice,
        LPCWSTR                   pSrcFile,
        LPDIRECT3DCUBETEXTURE9*   ppCubeTexture);

#ifdef UNICODE
#define D3DXCreateCubeTextureFromFile D3DXCreateCubeTextureFromFileW
#else
#define D3DXCreateCubeTextureFromFile D3DXCreateCubeTextureFromFileA
#endif


HRESULT WINAPI
    D3DXCreateVolumeTextureFromFileA(
        LPDIRECT3DDEVICE9         pDevice,
        LPCSTR                    pSrcFile,
        LPDIRECT3DVOLUMETEXTURE9* ppVolumeTexture);

HRESULT WINAPI
    D3DXCreateVolumeTextureFromFileW(
        LPDIRECT3DDEVICE9         pDevice,
        LPCWSTR                   pSrcFile,
        LPDIRECT3DVOLUMETEXTURE9* ppVolumeTexture);

#ifdef UNICODE
#define D3DXCreateVolumeTextureFromFile D3DXCreateVolumeTextureFromFileW
#else
#define D3DXCreateVolumeTextureFromFile D3DXCreateVolumeTextureFromFileA
#endif


// FromResource

HRESULT WINAPI
    D3DXCreateTextureFromResourceA(
        LPDIRECT3DDEVICE9         pDevice,
        HMODULE                   hSrcModule,
        LPCSTR                    pSrcResource,
        LPDIRECT3DTEXTURE9*       ppTexture);

HRESULT WINAPI
    D3DXCreateTextureFromResourceW(
        LPDIRECT3DDEVICE9         pDevice,
        HMODULE                   hSrcModule,
        LPCWSTR                   pSrcResource,
        LPDIRECT3DTEXTURE9*       ppTexture);

#ifdef UNICODE
#define D3DXCreateTextureFromResource D3DXCreateTextureFromResourceW
#else
#define D3DXCreateTextureFromResource D3DXCreateTextureFromResourceA
#endif


HRESULT WINAPI
    D3DXCreateCubeTextureFromResourceA(
        LPDIRECT3DDEVICE9         pDevice,
        HMODULE                   hSrcModule,
        LPCSTR                    pSrcResource,
        LPDIRECT3DCUBETEXTURE9*   ppCubeTexture);

HRESULT WINAPI
    D3DXCreateCubeTextureFromResourceW(
        LPDIRECT3DDEVICE9         pDevice,
        HMODULE                   hSrcModule,
        LPCWSTR                   pSrcResource,
        LPDIRECT3DCUBETEXTURE9*   ppCubeTexture);

#ifdef UNICODE
#define D3DXCreateCubeTextureFromResource D3DXCreateCubeTextureFromResourceW
#else
#define D3DXCreateCubeTextureFromResource D3DXCreateCubeTextureFromResourceA
#endif


HRESULT WINAPI
    D3DXCreateVolumeTextureFromResourceA(
        LPDIRECT3DDEVICE9         pDevice,
        HMODULE                   hSrcModule,
        LPCSTR                    pSrcResource,
        LPDIRECT3DVOLUMETEXTURE9* ppVolumeTexture);

HRESULT WINAPI
    D3DXCreateVolumeTextureFromResourceW(
        LPDIRECT3DDEVICE9         pDevice,
        HMODULE                   hSrcModule,
        LPCWSTR                   pSrcResource,
        LPDIRECT3DVOLUMETEXTURE9* ppVolumeTexture);

#ifdef UNICODE
#define D3DXCreateVolumeTextureFromResource D3DXCreateVolumeTextureFromResourceW
#else
#define D3DXCreateVolumeTextureFromResource D3DXCreateVolumeTextureFromResourceA
#endif


// FromFileEx

HRESULT WINAPI
    D3DXCreateTextureFromFileExA(
        LPDIRECT3DDEVICE9         pDevice,
        LPCSTR                    pSrcFile,
        UINT                      Width,
        UINT                      Height,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        DWORD                     Filter,
        DWORD                     MipFilter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo,
        PALETTEENTRY*             pPalette,
        LPDIRECT3DTEXTURE9*       ppTexture);

HRESULT WINAPI
    D3DXCreateTextureFromFileExW(
        LPDIRECT3DDEVICE9         pDevice,
        LPCWSTR                   pSrcFile,
        UINT                      Width,
        UINT                      Height,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        DWORD                     Filter,
        DWORD                     MipFilter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo,
        PALETTEENTRY*             pPalette,
        LPDIRECT3DTEXTURE9*       ppTexture);

#ifdef UNICODE
#define D3DXCreateTextureFromFileEx D3DXCreateTextureFromFileExW
#else
#define D3DXCreateTextureFromFileEx D3DXCreateTextureFromFileExA
#endif


HRESULT WINAPI
    D3DXCreateCubeTextureFromFileExA(
        LPDIRECT3DDEVICE9         pDevice,
        LPCSTR                    pSrcFile,
        UINT                      Size,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        DWORD                     Filter,
        DWORD                     MipFilter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo,
        PALETTEENTRY*             pPalette,
        LPDIRECT3DCUBETEXTURE9*   ppCubeTexture);

HRESULT WINAPI
    D3DXCreateCubeTextureFromFileExW(
        LPDIRECT3DDEVICE9         pDevice,
        LPCWSTR                   pSrcFile,
        UINT                      Size,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        DWORD                     Filter,
        DWORD                     MipFilter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo,
        PALETTEENTRY*             pPalette,
        LPDIRECT3DCUBETEXTURE9*   ppCubeTexture);

#ifdef UNICODE
#define D3DXCreateCubeTextureFromFileEx D3DXCreateCubeTextureFromFileExW
#else
#define D3DXCreateCubeTextureFromFileEx D3DXCreateCubeTextureFromFileExA
#endif


HRESULT WINAPI
    D3DXCreateVolumeTextureFromFileExA(
        LPDIRECT3DDEVICE9         pDevice,
        LPCSTR                    pSrcFile,
        UINT                      Width,
        UINT                      Height,
        UINT                      Depth,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        DWORD                     Filter,
        DWORD                     MipFilter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo,
        PALETTEENTRY*             pPalette,
        LPDIRECT3DVOLUMETEXTURE9* ppVolumeTexture);

HRESULT WINAPI
    D3DXCreateVolumeTextureFromFileExW(
        LPDIRECT3DDEVICE9         pDevice,
        LPCWSTR                   pSrcFile,
        UINT                      Width,
        UINT                      Height,
        UINT                      Depth,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        DWORD                     Filter,
        DWORD                     MipFilter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo,
        PALETTEENTRY*             pPalette,
        LPDIRECT3DVOLUMETEXTURE9* ppVolumeTexture);

#ifdef UNICODE
#define D3DXCreateVolumeTextureFromFileEx D3DXCreateVolumeTextureFromFileExW
#else
#define D3DXCreateVolumeTextureFromFileEx D3DXCreateVolumeTextureFromFileExA
#endif


// FromResourceEx

HRESULT WINAPI
    D3DXCreateTextureFromResourceExA(
        LPDIRECT3DDEVICE9         pDevice,
        HMODULE                   hSrcModule,
        LPCSTR                    pSrcResource,
        UINT                      Width,
        UINT                      Height,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        DWORD                     Filter,
        DWORD                     MipFilter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo,
        PALETTEENTRY*             pPalette,
        LPDIRECT3DTEXTURE9*       ppTexture);

HRESULT WINAPI
    D3DXCreateTextureFromResourceExW(
        LPDIRECT3DDEVICE9         pDevice,
        HMODULE                   hSrcModule,
        LPCWSTR                   pSrcResource,
        UINT                      Width,
        UINT                      Height,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        DWORD                     Filter,
        DWORD                     MipFilter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo,
        PALETTEENTRY*             pPalette,
        LPDIRECT3DTEXTURE9*       ppTexture);

#ifdef UNICODE
#define D3DXCreateTextureFromResourceEx D3DXCreateTextureFromResourceExW
#else
#define D3DXCreateTextureFromResourceEx D3DXCreateTextureFromResourceExA
#endif


HRESULT WINAPI
    D3DXCreateCubeTextureFromResourceExA(
        LPDIRECT3DDEVICE9         pDevice,
        HMODULE                   hSrcModule,
        LPCSTR                    pSrcResource,
        UINT                      Size,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        DWORD                     Filter,
        DWORD                     MipFilter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo,
        PALETTEENTRY*             pPalette,
        LPDIRECT3DCUBETEXTURE9*   ppCubeTexture);

HRESULT WINAPI
    D3DXCreateCubeTextureFromResourceExW(
        LPDIRECT3DDEVICE9         pDevice,
        HMODULE                   hSrcModule,
        LPCWSTR                   pSrcResource,
        UINT                      Size,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        DWORD                     Filter,
        DWORD                     MipFilter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo,
        PALETTEENTRY*             pPalette,
        LPDIRECT3DCUBETEXTURE9*   ppCubeTexture);

#ifdef UNICODE
#define D3DXCreateCubeTextureFromResourceEx D3DXCreateCubeTextureFromResourceExW
#else
#define D3DXCreateCubeTextureFromResourceEx D3DXCreateCubeTextureFromResourceExA
#endif


HRESULT WINAPI
    D3DXCreateVolumeTextureFromResourceExA(
        LPDIRECT3DDEVICE9         pDevice,
        HMODULE                   hSrcModule,
        LPCSTR                    pSrcResource,
        UINT                      Width,
        UINT                      Height,
        UINT                      Depth,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        DWORD                     Filter,
        DWORD                     MipFilter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo,
        PALETTEENTRY*             pPalette,
        LPDIRECT3DVOLUMETEXTURE9* ppVolumeTexture);

HRESULT WINAPI
    D3DXCreateVolumeTextureFromResourceExW(
        LPDIRECT3DDEVICE9         pDevice,
        HMODULE                   hSrcModule,
        LPCWSTR                   pSrcResource,
        UINT                      Width,
        UINT                      Height,
        UINT                      Depth,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        DWORD                     Filter,
        DWORD                     MipFilter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo,
        PALETTEENTRY*             pPalette,
        LPDIRECT3DVOLUMETEXTURE9* ppVolumeTexture);

#ifdef UNICODE
#define D3DXCreateVolumeTextureFromResourceEx D3DXCreateVolumeTextureFromResourceExW
#else
#define D3DXCreateVolumeTextureFromResourceEx D3DXCreateVolumeTextureFromResourceExA
#endif


// FromFileInMemory

HRESULT WINAPI
    D3DXCreateTextureFromFileInMemory(
        LPDIRECT3DDEVICE9         pDevice,
        LPCVOID                   pSrcData,
        UINT                      SrcDataSize,
        LPDIRECT3DTEXTURE9*       ppTexture);

HRESULT WINAPI
    D3DXCreateCubeTextureFromFileInMemory(
        LPDIRECT3DDEVICE9         pDevice,
        LPCVOID                   pSrcData,
        UINT                      SrcDataSize,
        LPDIRECT3DCUBETEXTURE9*   ppCubeTexture);

HRESULT WINAPI
    D3DXCreateVolumeTextureFromFileInMemory(
        LPDIRECT3DDEVICE9         pDevice,
        LPCVOID                   pSrcData,
        UINT                      SrcDataSize,
        LPDIRECT3DVOLUMETEXTURE9* ppVolumeTexture);


// FromFileInMemoryEx

HRESULT WINAPI
    D3DXCreateTextureFromFileInMemoryEx(
        LPDIRECT3DDEVICE9         pDevice,
        LPCVOID                   pSrcData,
        UINT                      SrcDataSize,
        UINT                      Width,
        UINT                      Height,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        DWORD                     Filter,
        DWORD                     MipFilter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo,
        PALETTEENTRY*             pPalette,
        LPDIRECT3DTEXTURE9*       ppTexture);

HRESULT WINAPI
    D3DXCreateCubeTextureFromFileInMemoryEx(
        LPDIRECT3DDEVICE9         pDevice,
        LPCVOID                   pSrcData,
        UINT                      SrcDataSize,
        UINT                      Size,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        DWORD                     Filter,
        DWORD                     MipFilter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo,
        PALETTEENTRY*             pPalette,
        LPDIRECT3DCUBETEXTURE9*   ppCubeTexture);

HRESULT WINAPI
    D3DXCreateVolumeTextureFromFileInMemoryEx(
        LPDIRECT3DDEVICE9         pDevice,
        LPCVOID                   pSrcData,
        UINT                      SrcDataSize,
        UINT                      Width,
        UINT                      Height,
        UINT                      Depth,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        DWORD                     Filter,
        DWORD                     MipFilter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo,
        PALETTEENTRY*             pPalette,
        LPDIRECT3DVOLUMETEXTURE9* ppVolumeTexture);



//----------------------------------------------------------------------------
// D3DXSaveTextureToFile:
// ----------------------
// Save a texture to a file.
//
// Parameters:
//  pDestFile
//      File name of the destination file
//  DestFormat
//      D3DXIMAGE_FILEFORMAT specifying file format to use when saving.
//  pSrcTexture
//      Source texture, containing the image to be saved
//  pSrcPalette
//      Source palette of 256 colors, or NULL
//
//----------------------------------------------------------------------------


HRESULT WINAPI
    D3DXSaveTextureToFileA(
        LPCSTR                    pDestFile,
        D3DXIMAGE_FILEFORMAT      DestFormat,
        LPDIRECT3DBASETEXTURE9    pSrcTexture,
        CONST PALETTEENTRY*       pSrcPalette);

HRESULT WINAPI
    D3DXSaveTextureToFileW(
        LPCWSTR                   pDestFile,
        D3DXIMAGE_FILEFORMAT      DestFormat,
        LPDIRECT3DBASETEXTURE9    pSrcTexture,
        CONST PALETTEENTRY*       pSrcPalette);

#ifdef UNICODE
#define D3DXSaveTextureToFile D3DXSaveTextureToFileW
#else
#define D3DXSaveTextureToFile D3DXSaveTextureToFileA
#endif


//----------------------------------------------------------------------------
// D3DXSaveTextureToFileInMemory:
// ----------------------
// Save a texture to a file.
//
// Parameters:
//  ppDestBuf
//      address of a d3dxbuffer pointer to return the image data
//  DestFormat
//      D3DXIMAGE_FILEFORMAT specifying file format to use when saving.
//  pSrcTexture
//      Source texture, containing the image to be saved
//  pSrcPalette
//      Source palette of 256 colors, or NULL
//
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXSaveTextureToFileInMemory(
        LPD3DXBUFFER*             ppDestBuf,
        D3DXIMAGE_FILEFORMAT      DestFormat,
        LPDIRECT3DBASETEXTURE9    pSrcTexture,
        CONST PALETTEENTRY*       pSrcPalette);




//////////////////////////////////////////////////////////////////////////////
// Misc Texture APIs /////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// D3DXFilterTexture:
// ------------------
// Filters mipmaps levels of a texture.
//
// Parameters:
//  pBaseTexture
//      The texture object to be filtered
//  pPalette
//      256 color palette to be used, or NULL for non-palettized formats
//  SrcLevel
//      The level whose image is used to generate the subsequent levels. 
//  Filter
//      D3DX_FILTER flags controlling how each miplevel is filtered.
//      Or D3DX_DEFAULT for D3DX_FILTER_BOX,
//
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXFilterTexture(
        LPDIRECT3DBASETEXTURE9    pBaseTexture,
        CONST PALETTEENTRY*       pPalette,
        UINT                      SrcLevel,
        DWORD                     Filter);

#define D3DXFilterCubeTexture D3DXFilterTexture
#define D3DXFilterVolumeTexture D3DXFilterTexture



//----------------------------------------------------------------------------
// D3DXFillTexture:
// ----------------
// Uses a user provided function to fill each texel of each mip level of a
// given texture.
//
// Paramters:
//  pTexture, pCubeTexture, pVolumeTexture
//      Pointer to the texture to be filled.
//  pFunction
//      Pointer to user provided evalutor function which will be used to 
//      compute the value of each texel.
//  pData
//      Pointer to an arbitrary block of user defined data.  This pointer 
//      will be passed to the function provided in pFunction
//-----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXFillTexture(
        LPDIRECT3DTEXTURE9        pTexture,
        LPD3DXFILL2D              pFunction,
        LPVOID                    pData);

HRESULT WINAPI
    D3DXFillCubeTexture(
        LPDIRECT3DCUBETEXTURE9    pCubeTexture,
        LPD3DXFILL3D              pFunction,
        LPVOID                    pData);

HRESULT WINAPI
    D3DXFillVolumeTexture(
        LPDIRECT3DVOLUMETEXTURE9  pVolumeTexture,
        LPD3DXFILL3D              pFunction,
        LPVOID                    pData);

//---------------------------------------------------------------------------
// D3DXFillTextureTX:
// ------------------
// Uses a TX Shader target to function to fill each texel of each mip level
// of a given texture. The TX Shader target should be a compiled function 
// taking 2 paramters and returning a float4 color.
//
// Paramters:
//  pTexture, pCubeTexture, pVolumeTexture
//      Pointer to the texture to be filled.
//  pTextureShader
//      Pointer to the texture shader to be used to fill in the texture
//----------------------------------------------------------------------------

HRESULT WINAPI 
    D3DXFillTextureTX(
        LPDIRECT3DTEXTURE9        pTexture,
        LPD3DXTEXTURESHADER       pTextureShader);


HRESULT WINAPI
    D3DXFillCubeTextureTX(
        LPDIRECT3DCUBETEXTURE9    pCubeTexture,
        LPD3DXTEXTURESHADER       pTextureShader);
                                                
                                                        
HRESULT WINAPI 
    D3DXFillVolumeTextureTX(
        LPDIRECT3DVOLUMETEXTURE9  pVolumeTexture,
        LPD3DXTEXTURESHADER       pTextureShader);



//----------------------------------------------------------------------------
// D3DXComputeNormalMap:
// ---------------------
// Converts a height map into a normal map.  The (x,y,z) components of each
// normal are mapped to the (r,g,b) channels of the output texture.
//
// Parameters
//  pTexture
//      Pointer to the destination texture
//  pSrcTexture
//      Pointer to the source heightmap texture 
//  pSrcPalette
//      Source palette of 256 colors, or NULL
//  Flags
//      D3DX_NORMALMAP flags
//  Channel
//      D3DX_CHANNEL specifying source of height information
//  Amplitude
//      The constant value which the height information is multiplied by.
//---------------------------------------------------------------------------

HRESULT WINAPI
    D3DXComputeNormalMap(
        LPDIRECT3DTEXTURE9        pTexture,
        LPDIRECT3DTEXTURE9        pSrcTexture,
        CONST PALETTEENTRY*       pSrcPalette,
        DWORD                     Flags,
        DWORD                     Channel,
        FLOAT                     Amplitude);




#ifdef __cplusplus
}
#endif //__cplusplus

#endif //__D3DX9TEX_H__


///////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) Microsoft Corporation.  All Rights Reserved.
//
//  File:       d3dx9core.h
//  Content:    D3DX core types and functions
//
///////////////////////////////////////////////////////////////////////////

#ifndef __D3DX9CORE_H__
#define __D3DX9CORE_H__


///////////////////////////////////////////////////////////////////////////
// D3DX_SDK_VERSION:
// -----------------
// This identifier is passed to D3DXCheckVersion in order to ensure that an
// application was built against the correct header files and lib files. 
// This number is incremented whenever a header (or other) change would 
// require applications to be rebuilt. If the version doesn't match, 
// D3DXCheckVersion will return FALSE. (The number itself has no meaning.)
///////////////////////////////////////////////////////////////////////////

#define D3DX_VERSION 0x0902

#define D3DX_SDK_VERSION 43

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

BOOL WINAPI
    D3DXCheckVersion(UINT D3DSdkVersion, UINT D3DXSdkVersion);

#ifdef __cplusplus
}
#endif //__cplusplus



///////////////////////////////////////////////////////////////////////////
// D3DXDebugMute
//    Mutes D3DX and D3D debug spew (TRUE - mute, FALSE - not mute)
//
//  returns previous mute value
//
///////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

BOOL WINAPI
    D3DXDebugMute(BOOL Mute);  

#ifdef __cplusplus
}
#endif //__cplusplus


///////////////////////////////////////////////////////////////////////////
// D3DXGetDriverLevel:
//    Returns driver version information:
//
//    700 - DX7 level driver
//    800 - DX8 level driver
//    900 - DX9 level driver
///////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

UINT WINAPI
    D3DXGetDriverLevel(LPDIRECT3DDEVICE9 pDevice);

#ifdef __cplusplus
}
#endif //__cplusplus


///////////////////////////////////////////////////////////////////////////
// ID3DXBuffer:
// ------------
// The buffer object is used by D3DX to return arbitrary size data.
//
// GetBufferPointer -
//    Returns a pointer to the beginning of the buffer.
//
// GetBufferSize -
//    Returns the size of the buffer, in bytes.
///////////////////////////////////////////////////////////////////////////

typedef interface ID3DXBuffer ID3DXBuffer;
typedef interface ID3DXBuffer *LPD3DXBUFFER;

// {8BA5FB08-5195-40e2-AC58-0D989C3A0102}
DEFINE_GUID(IID_ID3DXBuffer, 
0x8ba5fb08, 0x5195, 0x40e2, 0xac, 0x58, 0xd, 0x98, 0x9c, 0x3a, 0x1, 0x2);

#undef INTERFACE
#define INTERFACE ID3DXBuffer

DECLARE_INTERFACE_(ID3DXBuffer, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // ID3DXBuffer
    STDMETHOD_(LPVOID, GetBufferPointer)(THIS) PURE;
    STDMETHOD_(DWORD, GetBufferSize)(THIS) PURE;
};



//////////////////////////////////////////////////////////////////////////////
// D3DXSPRITE flags:
// -----------------
// D3DXSPRITE_DONOTSAVESTATE
//   Specifies device state is not to be saved and restored in Begin/End.
// D3DXSPRITE_DONOTMODIFY_RENDERSTATE
//   Specifies device render state is not to be changed in Begin.  The device
//   is assumed to be in a valid state to draw vertices containing POSITION0, 
//   TEXCOORD0, and COLOR0 data.
// D3DXSPRITE_OBJECTSPACE
//   The WORLD, VIEW, and PROJECTION transforms are NOT modified.  The 
//   transforms currently set to the device are used to transform the sprites 
//   when the batch is drawn (at Flush or End).  If this is not specified, 
//   WORLD, VIEW, and PROJECTION transforms are modified so that sprites are 
//   drawn in screenspace coordinates.
// D3DXSPRITE_BILLBOARD
//   Rotates each sprite about its center so that it is facing the viewer.
// D3DXSPRITE_ALPHABLEND
//   Enables ALPHABLEND(SRCALPHA, INVSRCALPHA) and ALPHATEST(alpha > 0).
//   ID3DXFont expects this to be set when drawing text.
// D3DXSPRITE_SORT_TEXTURE
//   Sprites are sorted by texture prior to drawing.  This is recommended when
//   drawing non-overlapping sprites of uniform depth.  For example, drawing
//   screen-aligned text with ID3DXFont.
// D3DXSPRITE_SORT_DEPTH_FRONTTOBACK
//   Sprites are sorted by depth front-to-back prior to drawing.  This is 
//   recommended when drawing opaque sprites of varying depths.
// D3DXSPRITE_SORT_DEPTH_BACKTOFRONT
//   Sprites are sorted by depth back-to-front prior to drawing.  This is 
//   recommended when drawing transparent sprites of varying depths.
// D3DXSPRITE_DO_NOT_ADDREF_TEXTURE
//   Disables calling AddRef() on every draw, and Release() on Flush() for
//   better performance.
//////////////////////////////////////////////////////////////////////////////

#define D3DXSPRITE_DONOTSAVESTATE               (1 << 0)
#define D3DXSPRITE_DONOTMODIFY_RENDERSTATE      (1 << 1)
#define D3DXSPRITE_OBJECTSPACE                  (1 << 2)
#define D3DXSPRITE_BILLBOARD                    (1 << 3)
#define D3DXSPRITE_ALPHABLEND                   (1 << 4)
#define D3DXSPRITE_SORT_TEXTURE                 (1 << 5)
#define D3DXSPRITE_SORT_DEPTH_FRONTTOBACK       (1 << 6)
#define D3DXSPRITE_SORT_DEPTH_BACKTOFRONT       (1 << 7)
#define D3DXSPRITE_DO_NOT_ADDREF_TEXTURE        (1 << 8)


//////////////////////////////////////////////////////////////////////////////
// ID3DXSprite:
// ------------
// This object intends to provide an easy way to drawing sprites using D3D.
//
// Begin - 
//    Prepares device for drawing sprites.
//
// Draw -
//    Draws a sprite.  Before transformation, the sprite is the size of 
//    SrcRect, with its top-left corner specified by Position.  The color 
//    and alpha channels are modulated by Color.
//
// Flush -
//    Forces all batched sprites to submitted to the device.
//
// End - 
//    Restores device state to how it was when Begin was called.
//
// OnLostDevice, OnResetDevice -
//    Call OnLostDevice() on this object before calling Reset() on the
//    device, so that this object can release any stateblocks and video
//    memory resources.  After Reset(), the call OnResetDevice().
//////////////////////////////////////////////////////////////////////////////

typedef interface ID3DXSprite ID3DXSprite;
typedef interface ID3DXSprite *LPD3DXSPRITE;


// {BA0B762D-7D28-43ec-B9DC-2F84443B0614}
DEFINE_GUID(IID_ID3DXSprite, 
0xba0b762d, 0x7d28, 0x43ec, 0xb9, 0xdc, 0x2f, 0x84, 0x44, 0x3b, 0x6, 0x14);


#undef INTERFACE
#define INTERFACE ID3DXSprite

DECLARE_INTERFACE_(ID3DXSprite, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // ID3DXSprite
    STDMETHOD(GetDevice)(THIS_ LPDIRECT3DDEVICE9* ppDevice) PURE;

    STDMETHOD(GetTransform)(THIS_ D3DXMATRIX *pTransform) PURE;
    STDMETHOD(SetTransform)(THIS_ CONST D3DXMATRIX *pTransform) PURE;

    STDMETHOD(SetWorldViewRH)(THIS_ CONST D3DXMATRIX *pWorld, CONST D3DXMATRIX *pView) PURE;
    STDMETHOD(SetWorldViewLH)(THIS_ CONST D3DXMATRIX *pWorld, CONST D3DXMATRIX *pView) PURE;

    STDMETHOD(Begin)(THIS_ DWORD Flags) PURE;
    STDMETHOD(Draw)(THIS_ LPDIRECT3DTEXTURE9 pTexture, CONST RECT *pSrcRect, CONST D3DXVECTOR3 *pCenter, CONST D3DXVECTOR3 *pPosition, D3DCOLOR Color) PURE;
    STDMETHOD(Flush)(THIS) PURE;
    STDMETHOD(End)(THIS) PURE;

    STDMETHOD(OnLostDevice)(THIS) PURE;
    STDMETHOD(OnResetDevice)(THIS) PURE;
};


#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

HRESULT WINAPI 
    D3DXCreateSprite( 
        LPDIRECT3DDEVICE9   pDevice, 
        LPD3DXSPRITE*       ppSprite);

#ifdef __cplusplus
}
#endif //__cplusplus



//////////////////////////////////////////////////////////////////////////////
// ID3DXFont:
// ----------
// Font objects contain the textures and resources needed to render a specific 
// font on a specific device.
//
// GetGlyphData -
//    Returns glyph cache data, for a given glyph.
//
// PreloadCharacters/PreloadGlyphs/PreloadText -
//    Preloads glyphs into the glyph cache textures.
//
// DrawText -
//    Draws formatted text on a D3D device.  Some parameters are 
//    surprisingly similar to those of GDI's DrawText function.  See GDI 
//    documentation for a detailed description of these parameters.
//    If pSprite is NULL, an internal sprite object will be used.
//
// OnLostDevice, OnResetDevice -
//    Call OnLostDevice() on this object before calling Reset() on the
//    device, so that this object can release any stateblocks and video
//    memory resources.  After Reset(), the call OnResetDevice().
//////////////////////////////////////////////////////////////////////////////

typedef struct _D3DXFONT_DESCA
{
    INT Height;
    UINT Width;
    UINT Weight;
    UINT MipLevels;
    BOOL Italic;
    BYTE CharSet;
    BYTE OutputPrecision;
    BYTE Quality;
    BYTE PitchAndFamily;
    CHAR FaceName[LF_FACESIZE];

} D3DXFONT_DESCA, *LPD3DXFONT_DESCA;

typedef struct _D3DXFONT_DESCW
{
    INT Height;
    UINT Width;
    UINT Weight;
    UINT MipLevels;
    BOOL Italic;
    BYTE CharSet;
    BYTE OutputPrecision;
    BYTE Quality;
    BYTE PitchAndFamily;
    WCHAR FaceName[LF_FACESIZE];

} D3DXFONT_DESCW, *LPD3DXFONT_DESCW;

#ifdef UNICODE
typedef D3DXFONT_DESCW D3DXFONT_DESC;
typedef LPD3DXFONT_DESCW LPD3DXFONT_DESC;
#else
typedef D3DXFONT_DESCA D3DXFONT_DESC;
typedef LPD3DXFONT_DESCA LPD3DXFONT_DESC;
#endif


typedef interface ID3DXFont ID3DXFont;
typedef interface ID3DXFont *LPD3DXFONT;


// {D79DBB70-5F21-4d36-BBC2-FF525C213CDC}
DEFINE_GUID(IID_ID3DXFont, 
0xd79dbb70, 0x5f21, 0x4d36, 0xbb, 0xc2, 0xff, 0x52, 0x5c, 0x21, 0x3c, 0xdc);


#undef INTERFACE
#define INTERFACE ID3DXFont

DECLARE_INTERFACE_(ID3DXFont, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // ID3DXFont
    STDMETHOD(GetDevice)(THIS_ LPDIRECT3DDEVICE9 *ppDevice) PURE;
    STDMETHOD(GetDescA)(THIS_ D3DXFONT_DESCA *pDesc) PURE;
    STDMETHOD(GetDescW)(THIS_ D3DXFONT_DESCW *pDesc) PURE;
    STDMETHOD_(BOOL, GetTextMetricsA)(THIS_ TEXTMETRICA *pTextMetrics) PURE;
    STDMETHOD_(BOOL, GetTextMetricsW)(THIS_ TEXTMETRICW *pTextMetrics) PURE;

    STDMETHOD_(HDC, GetDC)(THIS) PURE;
    STDMETHOD(GetGlyphData)(THIS_ UINT Glyph, LPDIRECT3DTEXTURE9 *ppTexture, RECT *pBlackBox, POINT *pCellInc) PURE;

    STDMETHOD(PreloadCharacters)(THIS_ UINT First, UINT Last) PURE;
    STDMETHOD(PreloadGlyphs)(THIS_ UINT First, UINT Last) PURE;
    STDMETHOD(PreloadTextA)(THIS_ LPCSTR pString, INT Count) PURE;
    STDMETHOD(PreloadTextW)(THIS_ LPCWSTR pString, INT Count) PURE;

    STDMETHOD_(INT, DrawTextA)(THIS_ LPD3DXSPRITE pSprite, LPCSTR pString, INT Count, LPRECT pRect, DWORD Format, D3DCOLOR Color) PURE;
    STDMETHOD_(INT, DrawTextW)(THIS_ LPD3DXSPRITE pSprite, LPCWSTR pString, INT Count, LPRECT pRect, DWORD Format, D3DCOLOR Color) PURE;

    STDMETHOD(OnLostDevice)(THIS) PURE;
    STDMETHOD(OnResetDevice)(THIS) PURE;

#ifdef __cplusplus
#ifdef UNICODE
    HRESULT GetDesc(D3DXFONT_DESCW *pDesc) { return GetDescW(pDesc); }
    HRESULT PreloadText(LPCWSTR pString, INT Count) { return PreloadTextW(pString, Count); }
#else
    HRESULT GetDesc(D3DXFONT_DESCA *pDesc) { return GetDescA(pDesc); }
    HRESULT PreloadText(LPCSTR pString, INT Count) { return PreloadTextA(pString, Count); }
#endif
#endif //__cplusplus
};

#ifndef GetTextMetrics
#ifdef UNICODE
#define GetTextMetrics GetTextMetricsW
#else
#define GetTextMetrics GetTextMetricsA
#endif
#endif

#ifndef DrawText
#ifdef UNICODE
#define DrawText DrawTextW
#else
#define DrawText DrawTextA
#endif
#endif


#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


HRESULT WINAPI 
    D3DXCreateFontA(
        LPDIRECT3DDEVICE9       pDevice,  
        INT                     Height,
        UINT                    Width,
        UINT                    Weight,
        UINT                    MipLevels,
        BOOL                    Italic,
        DWORD                   CharSet,
        DWORD                   OutputPrecision,
        DWORD                   Quality,
        DWORD                   PitchAndFamily,
        LPCSTR                  pFaceName,
        LPD3DXFONT*             ppFont);

HRESULT WINAPI 
    D3DXCreateFontW(
        LPDIRECT3DDEVICE9       pDevice,  
        INT                     Height,
        UINT                    Width,
        UINT                    Weight,
        UINT                    MipLevels,
        BOOL                    Italic,
        DWORD                   CharSet,
        DWORD                   OutputPrecision,
        DWORD                   Quality,
        DWORD                   PitchAndFamily,
        LPCWSTR                 pFaceName,
        LPD3DXFONT*             ppFont);

#ifdef UNICODE
#define D3DXCreateFont D3DXCreateFontW
#else
#define D3DXCreateFont D3DXCreateFontA
#endif


HRESULT WINAPI 
    D3DXCreateFontIndirectA( 
        LPDIRECT3DDEVICE9       pDevice, 
        CONST D3DXFONT_DESCA*   pDesc, 
        LPD3DXFONT*             ppFont);

HRESULT WINAPI 
    D3DXCreateFontIndirectW( 
        LPDIRECT3DDEVICE9       pDevice, 
        CONST D3DXFONT_DESCW*   pDesc, 
        LPD3DXFONT*             ppFont);

#ifdef UNICODE
#define D3DXCreateFontIndirect D3DXCreateFontIndirectW
#else
#define D3DXCreateFontIndirect D3DXCreateFontIndirectA
#endif


#ifdef __cplusplus
}
#endif //__cplusplus



///////////////////////////////////////////////////////////////////////////
// ID3DXRenderToSurface:
// ---------------------
// This object abstracts rendering to surfaces.  These surfaces do not 
// necessarily need to be render targets.  If they are not, a compatible
// render target is used, and the result copied into surface at end scene.
//
// BeginScene, EndScene -
//    Call BeginScene() and EndScene() at the beginning and ending of your
//    scene.  These calls will setup and restore render targets, viewports, 
//    etc.. 
//
// OnLostDevice, OnResetDevice -
//    Call OnLostDevice() on this object before calling Reset() on the
//    device, so that this object can release any stateblocks and video
//    memory resources.  After Reset(), the call OnResetDevice().
///////////////////////////////////////////////////////////////////////////

typedef struct _D3DXRTS_DESC
{
    UINT                Width;
    UINT                Height;
    D3DFORMAT           Format;
    BOOL                DepthStencil;
    D3DFORMAT           DepthStencilFormat;

} D3DXRTS_DESC, *LPD3DXRTS_DESC;


typedef interface ID3DXRenderToSurface ID3DXRenderToSurface;
typedef interface ID3DXRenderToSurface *LPD3DXRENDERTOSURFACE;


// {6985F346-2C3D-43b3-BE8B-DAAE8A03D894}
DEFINE_GUID(IID_ID3DXRenderToSurface, 
0x6985f346, 0x2c3d, 0x43b3, 0xbe, 0x8b, 0xda, 0xae, 0x8a, 0x3, 0xd8, 0x94);


#undef INTERFACE
#define INTERFACE ID3DXRenderToSurface

DECLARE_INTERFACE_(ID3DXRenderToSurface, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // ID3DXRenderToSurface
    STDMETHOD(GetDevice)(THIS_ LPDIRECT3DDEVICE9* ppDevice) PURE;
    STDMETHOD(GetDesc)(THIS_ D3DXRTS_DESC* pDesc) PURE;

    STDMETHOD(BeginScene)(THIS_ LPDIRECT3DSURFACE9 pSurface, CONST D3DVIEWPORT9* pViewport) PURE;
    STDMETHOD(EndScene)(THIS_ DWORD MipFilter) PURE;

    STDMETHOD(OnLostDevice)(THIS) PURE;
    STDMETHOD(OnResetDevice)(THIS) PURE;
};


#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

HRESULT WINAPI
    D3DXCreateRenderToSurface(
        LPDIRECT3DDEVICE9       pDevice,
        UINT                    Width,
        UINT                    Height,
        D3DFORMAT               Format,
        BOOL                    DepthStencil,
        D3DFORMAT               DepthStencilFormat,
        LPD3DXRENDERTOSURFACE*  ppRenderToSurface);

#ifdef __cplusplus
}
#endif //__cplusplus



///////////////////////////////////////////////////////////////////////////
// ID3DXRenderToEnvMap:
// --------------------
// This object abstracts rendering to environment maps.  These surfaces 
// do not necessarily need to be render targets.  If they are not, a 
// compatible render target is used, and the result copied into the
// environment map at end scene.
//
// BeginCube, BeginSphere, BeginHemisphere, BeginParabolic -
//    This function initiates the rendering of the environment map.  As
//    parameters, you pass the textures in which will get filled in with
//    the resulting environment map.
//
// Face -
//    Call this function to initiate the drawing of each face.  For each 
//    environment map, you will call this six times.. once for each face 
//    in D3DCUBEMAP_FACES.
//
// End -
//    This will restore all render targets, and if needed compose all the
//    rendered faces into the environment map surfaces.
//
// OnLostDevice, OnResetDevice -
//    Call OnLostDevice() on this object before calling Reset() on the
//    device, so that this object can release any stateblocks and video
//    memory resources.  After Reset(), the call OnResetDevice().
///////////////////////////////////////////////////////////////////////////

typedef struct _D3DXRTE_DESC
{
    UINT        Size;
    UINT        MipLevels;
    D3DFORMAT   Format;
    BOOL        DepthStencil;
    D3DFORMAT   DepthStencilFormat;

} D3DXRTE_DESC, *LPD3DXRTE_DESC;


typedef interface ID3DXRenderToEnvMap ID3DXRenderToEnvMap;
typedef interface ID3DXRenderToEnvMap *LPD3DXRenderToEnvMap;


// {313F1B4B-C7B0-4fa2-9D9D-8D380B64385E}
DEFINE_GUID(IID_ID3DXRenderToEnvMap, 
0x313f1b4b, 0xc7b0, 0x4fa2, 0x9d, 0x9d, 0x8d, 0x38, 0xb, 0x64, 0x38, 0x5e);


#undef INTERFACE
#define INTERFACE ID3DXRenderToEnvMap

DECLARE_INTERFACE_(ID3DXRenderToEnvMap, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // ID3DXRenderToEnvMap
    STDMETHOD(GetDevice)(THIS_ LPDIRECT3DDEVICE9* ppDevice) PURE;
    STDMETHOD(GetDesc)(THIS_ D3DXRTE_DESC* pDesc) PURE;

    STDMETHOD(BeginCube)(THIS_ 
        LPDIRECT3DCUBETEXTURE9 pCubeTex) PURE;

    STDMETHOD(BeginSphere)(THIS_
        LPDIRECT3DTEXTURE9 pTex) PURE;

    STDMETHOD(BeginHemisphere)(THIS_ 
        LPDIRECT3DTEXTURE9 pTexZPos,
        LPDIRECT3DTEXTURE9 pTexZNeg) PURE;

    STDMETHOD(BeginParabolic)(THIS_ 
        LPDIRECT3DTEXTURE9 pTexZPos,
        LPDIRECT3DTEXTURE9 pTexZNeg) PURE;

    STDMETHOD(Face)(THIS_ D3DCUBEMAP_FACES Face, DWORD MipFilter) PURE;
    STDMETHOD(End)(THIS_ DWORD MipFilter) PURE;

    STDMETHOD(OnLostDevice)(THIS) PURE;
    STDMETHOD(OnResetDevice)(THIS) PURE;
};


#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

HRESULT WINAPI
    D3DXCreateRenderToEnvMap(
        LPDIRECT3DDEVICE9       pDevice,
        UINT                    Size,
        UINT                    MipLevels,
        D3DFORMAT               Format,
        BOOL                    DepthStencil,
        D3DFORMAT               DepthStencilFormat,
        LPD3DXRenderToEnvMap*   ppRenderToEnvMap);

#ifdef __cplusplus
}
#endif //__cplusplus



///////////////////////////////////////////////////////////////////////////
// ID3DXLine:
// ------------
// This object intends to provide an easy way to draw lines using D3D.
//
// Begin - 
//    Prepares device for drawing lines
//
// Draw -
//    Draws a line strip in screen-space.
//    Input is in the form of a array defining points on the line strip. of D3DXVECTOR2 
//
// DrawTransform -
//    Draws a line in screen-space with a specified input transformation matrix.
//
// End - 
//     Restores device state to how it was when Begin was called.
//
// SetPattern - 
//     Applies a stipple pattern to the line.  Input is one 32-bit
//     DWORD which describes the stipple pattern. 1 is opaque, 0 is
//     transparent.
//
// SetPatternScale - 
//     Stretches the stipple pattern in the u direction.  Input is one
//     floating-point value.  0.0f is no scaling, whereas 1.0f doubles
//     the length of the stipple pattern.
//
// SetWidth - 
//     Specifies the thickness of the line in the v direction.  Input is
//     one floating-point value.
//
// SetAntialias - 
//     Toggles line antialiasing.  Input is a BOOL.
//     TRUE  = Antialiasing on.
//     FALSE = Antialiasing off.
//
// SetGLLines - 
//     Toggles non-antialiased OpenGL line emulation.  Input is a BOOL.
//     TRUE  = OpenGL line emulation on.
//     FALSE = OpenGL line emulation off.
//
// OpenGL line:     Regular line:  
//   *\                *\
//   | \              /  \
//   |  \            *\   \
//   *\  \             \   \
//     \  \             \   \
//      \  *             \   *
//       \ |              \ /
//        \|               *
//         *
//
// OnLostDevice, OnResetDevice -
//    Call OnLostDevice() on this object before calling Reset() on the
//    device, so that this object can release any stateblocks and video
//    memory resources.  After Reset(), the call OnResetDevice().
///////////////////////////////////////////////////////////////////////////


typedef interface ID3DXLine ID3DXLine;
typedef interface ID3DXLine *LPD3DXLINE;


// {D379BA7F-9042-4ac4-9F5E-58192A4C6BD8}
DEFINE_GUID(IID_ID3DXLine, 
0xd379ba7f, 0x9042, 0x4ac4, 0x9f, 0x5e, 0x58, 0x19, 0x2a, 0x4c, 0x6b, 0xd8);

#undef INTERFACE
#define INTERFACE ID3DXLine

DECLARE_INTERFACE_(ID3DXLine, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // ID3DXLine
    STDMETHOD(GetDevice)(THIS_ LPDIRECT3DDEVICE9* ppDevice) PURE;

    STDMETHOD(Begin)(THIS) PURE;

    STDMETHOD(Draw)(THIS_ CONST D3DXVECTOR2 *pVertexList,
        DWORD dwVertexListCount, D3DCOLOR Color) PURE;

    STDMETHOD(DrawTransform)(THIS_ CONST D3DXVECTOR3 *pVertexList,
        DWORD dwVertexListCount, CONST D3DXMATRIX* pTransform, 
        D3DCOLOR Color) PURE;

    STDMETHOD(SetPattern)(THIS_ DWORD dwPattern) PURE;
    STDMETHOD_(DWORD, GetPattern)(THIS) PURE;

    STDMETHOD(SetPatternScale)(THIS_ FLOAT fPatternScale) PURE;
    STDMETHOD_(FLOAT, GetPatternScale)(THIS) PURE;

    STDMETHOD(SetWidth)(THIS_ FLOAT fWidth) PURE;
    STDMETHOD_(FLOAT, GetWidth)(THIS) PURE;

    STDMETHOD(SetAntialias)(THIS_ BOOL bAntialias) PURE;
    STDMETHOD_(BOOL, GetAntialias)(THIS) PURE;

    STDMETHOD(SetGLLines)(THIS_ BOOL bGLLines) PURE;
    STDMETHOD_(BOOL, GetGLLines)(THIS) PURE;

    STDMETHOD(End)(THIS) PURE;

    STDMETHOD(OnLostDevice)(THIS) PURE;
    STDMETHOD(OnResetDevice)(THIS) PURE;
};


#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


HRESULT WINAPI
    D3DXCreateLine(
        LPDIRECT3DDEVICE9   pDevice,
        LPD3DXLINE*         ppLine);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //__D3DX9CORE_H__


///////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) Microsoft Corporation.  All Rights Reserved.
//
//  File:       d3dx9shapes.h
//  Content:    D3DX simple shapes
//
///////////////////////////////////////////////////////////////////////////

#ifndef __D3DX9SHAPES_H__
#define __D3DX9SHAPES_H__

///////////////////////////////////////////////////////////////////////////
// Functions:
///////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


//-------------------------------------------------------------------------
// D3DXCreatePolygon: 
// ------------------
// Creates a mesh containing an n-sided polygon.  The polygon is centered
// at the origin.
//
// Parameters:
//
//  pDevice     The D3D device with which the mesh is going to be used.
//  Length      Length of each side.
//  Sides       Number of sides the polygon has.  (Must be >= 3)
//  ppMesh      The mesh object which will be created
//  ppAdjacency Returns a buffer containing adjacency info.  Can be NULL.
//-------------------------------------------------------------------------
HRESULT WINAPI 
    D3DXCreatePolygon(
        LPDIRECT3DDEVICE9   pDevice,
        FLOAT               Length, 
        UINT                Sides, 
        LPD3DXMESH*         ppMesh,
        LPD3DXBUFFER*       ppAdjacency);


//-------------------------------------------------------------------------
// D3DXCreateBox: 
// --------------
// Creates a mesh containing an axis-aligned box.  The box is centered at
// the origin.
//
// Parameters:
//
//  pDevice     The D3D device with which the mesh is going to be used.
//  Width       Width of box (along X-axis)
//  Height      Height of box (along Y-axis)
//  Depth       Depth of box (along Z-axis)
//  ppMesh      The mesh object which will be created
//  ppAdjacency Returns a buffer containing adjacency info.  Can be NULL.
//-------------------------------------------------------------------------
HRESULT WINAPI 
    D3DXCreateBox(
        LPDIRECT3DDEVICE9   pDevice, 
        FLOAT               Width,
        FLOAT               Height,
        FLOAT               Depth,
        LPD3DXMESH*         ppMesh,
        LPD3DXBUFFER*       ppAdjacency);


//-------------------------------------------------------------------------
// D3DXCreateCylinder:
// -------------------
// Creates a mesh containing a cylinder.  The generated cylinder is
// centered at the origin, and its axis is aligned with the Z-axis.
//
// Parameters:
//
//  pDevice     The D3D device with which the mesh is going to be used.
//  Radius1     Radius at -Z end (should be >= 0.0f)
//  Radius2     Radius at +Z end (should be >= 0.0f)
//  Length      Length of cylinder (along Z-axis)
//  Slices      Number of slices about the main axis
//  Stacks      Number of stacks along the main axis
//  ppMesh      The mesh object which will be created
//  ppAdjacency Returns a buffer containing adjacency info.  Can be NULL.
//-------------------------------------------------------------------------
HRESULT WINAPI 
    D3DXCreateCylinder(
        LPDIRECT3DDEVICE9   pDevice,
        FLOAT               Radius1, 
        FLOAT               Radius2, 
        FLOAT               Length, 
        UINT                Slices, 
        UINT                Stacks,   
        LPD3DXMESH*         ppMesh,
        LPD3DXBUFFER*       ppAdjacency);


//-------------------------------------------------------------------------
// D3DXCreateSphere:
// -----------------
// Creates a mesh containing a sphere.  The sphere is centered at the
// origin.
//
// Parameters:
//
//  pDevice     The D3D device with which the mesh is going to be used.
//  Radius      Radius of the sphere (should be >= 0.0f)
//  Slices      Number of slices about the main axis
//  Stacks      Number of stacks along the main axis
//  ppMesh      The mesh object which will be created
//  ppAdjacency Returns a buffer containing adjacency info.  Can be NULL.
//-------------------------------------------------------------------------
HRESULT WINAPI
    D3DXCreateSphere(
        LPDIRECT3DDEVICE9  pDevice, 
        FLOAT              Radius, 
        UINT               Slices, 
        UINT               Stacks,
        LPD3DXMESH*        ppMesh,
        LPD3DXBUFFER*      ppAdjacency);


//-------------------------------------------------------------------------
// D3DXCreateTorus:
// ----------------
// Creates a mesh containing a torus.  The generated torus is centered at
// the origin, and its axis is aligned with the Z-axis.
//
// Parameters: 
//
//  pDevice     The D3D device with which the mesh is going to be used.
//  InnerRadius Inner radius of the torus (should be >= 0.0f)
//  OuterRadius Outer radius of the torue (should be >= 0.0f)
//  Sides       Number of sides in a cross-section (must be >= 3)
//  Rings       Number of rings making up the torus (must be >= 3)
//  ppMesh      The mesh object which will be created
//  ppAdjacency Returns a buffer containing adjacency info.  Can be NULL.
//-------------------------------------------------------------------------
HRESULT WINAPI
    D3DXCreateTorus(
        LPDIRECT3DDEVICE9   pDevice,
        FLOAT               InnerRadius,
        FLOAT               OuterRadius, 
        UINT                Sides,
        UINT                Rings, 
        LPD3DXMESH*         ppMesh,
        LPD3DXBUFFER*       ppAdjacency);


//-------------------------------------------------------------------------
// D3DXCreateTeapot: 
// -----------------
// Creates a mesh containing a teapot.
//
// Parameters: 
//
//  pDevice     The D3D device with which the mesh is going to be used.
//  ppMesh      The mesh object which will be created
//  ppAdjacency Returns a buffer containing adjacency info.  Can be NULL.
//-------------------------------------------------------------------------
HRESULT WINAPI
    D3DXCreateTeapot(
        LPDIRECT3DDEVICE9   pDevice,
        LPD3DXMESH*         ppMesh,
        LPD3DXBUFFER*       ppAdjacency);


//-------------------------------------------------------------------------
// D3DXCreateText: 
// --------------- 
// Creates a mesh containing the specified text using the font associated
// with the device context.
//
// Parameters:
//
//  pDevice       The D3D device with which the mesh is going to be used.
//  hDC           Device context, with desired font selected
//  pText         Text to generate
//  Deviation     Maximum chordal deviation from true font outlines
//  Extrusion     Amount to extrude text in -Z direction
//  ppMesh        The mesh object which will be created
//  pGlyphMetrics Address of buffer to receive glyph metric data (or NULL)
//-------------------------------------------------------------------------
HRESULT WINAPI
    D3DXCreateTextA(
        LPDIRECT3DDEVICE9   pDevice,
        HDC                 hDC,
        LPCSTR              pText,
        FLOAT               Deviation,
        FLOAT               Extrusion,
        LPD3DXMESH*         ppMesh,
        LPD3DXBUFFER*       ppAdjacency,
        LPGLYPHMETRICSFLOAT pGlyphMetrics);

HRESULT WINAPI
    D3DXCreateTextW(
        LPDIRECT3DDEVICE9   pDevice,
        HDC                 hDC,
        LPCWSTR             pText,
        FLOAT               Deviation,
        FLOAT               Extrusion,
        LPD3DXMESH*         ppMesh,
        LPD3DXBUFFER*       ppAdjacency,
        LPGLYPHMETRICSFLOAT pGlyphMetrics);

#ifdef UNICODE
#define D3DXCreateText D3DXCreateTextW
#else
#define D3DXCreateText D3DXCreateTextA
#endif


#ifdef __cplusplus
}
#endif //__cplusplus    

#endif //__D3DX9SHAPES_H__

//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) Microsoft Corporation.  All Rights Reserved.
//
//  File:       d3dx9tex.h
//  Content:    D3DX texturing APIs
//
//////////////////////////////////////////////////////////////////////////////

#ifndef __D3DX9TEX_H__
#define __D3DX9TEX_H__


//----------------------------------------------------------------------------
// D3DX_FILTER flags:
// ------------------
//
// A valid filter must contain one of these values:
//
//  D3DX_FILTER_NONE
//      No scaling or filtering will take place.  Pixels outside the bounds
//      of the source image are assumed to be transparent black.
//  D3DX_FILTER_POINT
//      Each destination pixel is computed by sampling the nearest pixel
//      from the source image.
//  D3DX_FILTER_LINEAR
//      Each destination pixel is computed by linearly interpolating between
//      the nearest pixels in the source image.  This filter works best 
//      when the scale on each axis is less than 2.
//  D3DX_FILTER_TRIANGLE
//      Every pixel in the source image contributes equally to the
//      destination image.  This is the slowest of all the filters.
//  D3DX_FILTER_BOX
//      Each pixel is computed by averaging a 2x2(x2) box pixels from 
//      the source image. Only works when the dimensions of the 
//      destination are half those of the source. (as with mip maps)
//
// And can be OR'd with any of these optional flags:
//
//  D3DX_FILTER_MIRROR_U
//      Indicates that pixels off the edge of the texture on the U-axis
//      should be mirrored, not wraped.
//  D3DX_FILTER_MIRROR_V
//      Indicates that pixels off the edge of the texture on the V-axis
//      should be mirrored, not wraped.
//  D3DX_FILTER_MIRROR_W
//      Indicates that pixels off the edge of the texture on the W-axis
//      should be mirrored, not wraped.
//  D3DX_FILTER_MIRROR
//      Same as specifying D3DX_FILTER_MIRROR_U | D3DX_FILTER_MIRROR_V |
//      D3DX_FILTER_MIRROR_V
//  D3DX_FILTER_DITHER
//      Dithers the resulting image using a 4x4 order dither pattern.
//  D3DX_FILTER_SRGB_IN
//      Denotes that the input data is in sRGB (gamma 2.2) colorspace.
//  D3DX_FILTER_SRGB_OUT
//      Denotes that the output data is in sRGB (gamma 2.2) colorspace.
//  D3DX_FILTER_SRGB
//      Same as specifying D3DX_FILTER_SRGB_IN | D3DX_FILTER_SRGB_OUT
//
//----------------------------------------------------------------------------

#define D3DX_FILTER_NONE             (1 << 0)
#define D3DX_FILTER_POINT            (2 << 0)
#define D3DX_FILTER_LINEAR           (3 << 0)
#define D3DX_FILTER_TRIANGLE         (4 << 0)
#define D3DX_FILTER_BOX              (5 << 0)

#define D3DX_FILTER_MIRROR_U         (1 << 16)
#define D3DX_FILTER_MIRROR_V         (2 << 16)
#define D3DX_FILTER_MIRROR_W         (4 << 16)
#define D3DX_FILTER_MIRROR           (7 << 16)

#define D3DX_FILTER_DITHER           (1 << 19)
#define D3DX_FILTER_DITHER_DIFFUSION (2 << 19)

#define D3DX_FILTER_SRGB_IN          (1 << 21)
#define D3DX_FILTER_SRGB_OUT         (2 << 21)
#define D3DX_FILTER_SRGB             (3 << 21)


//-----------------------------------------------------------------------------
// D3DX_SKIP_DDS_MIP_LEVELS is used to skip mip levels when loading a DDS file:
//-----------------------------------------------------------------------------

#define D3DX_SKIP_DDS_MIP_LEVELS_MASK   0x1F
#define D3DX_SKIP_DDS_MIP_LEVELS_SHIFT  26
#define D3DX_SKIP_DDS_MIP_LEVELS(levels, filter) ((((levels) & D3DX_SKIP_DDS_MIP_LEVELS_MASK) << D3DX_SKIP_DDS_MIP_LEVELS_SHIFT) | ((filter) == D3DX_DEFAULT ? D3DX_FILTER_BOX : (filter)))




//----------------------------------------------------------------------------
// D3DX_NORMALMAP flags:
// ---------------------
// These flags are used to control how D3DXComputeNormalMap generates normal
// maps.  Any number of these flags may be OR'd together in any combination.
//
//  D3DX_NORMALMAP_MIRROR_U
//      Indicates that pixels off the edge of the texture on the U-axis
//      should be mirrored, not wraped.
//  D3DX_NORMALMAP_MIRROR_V
//      Indicates that pixels off the edge of the texture on the V-axis
//      should be mirrored, not wraped.
//  D3DX_NORMALMAP_MIRROR
//      Same as specifying D3DX_NORMALMAP_MIRROR_U | D3DX_NORMALMAP_MIRROR_V
//  D3DX_NORMALMAP_INVERTSIGN
//      Inverts the direction of each normal 
//  D3DX_NORMALMAP_COMPUTE_OCCLUSION
//      Compute the per pixel Occlusion term and encodes it into the alpha.
//      An Alpha of 1 means that the pixel is not obscured in anyway, and
//      an alpha of 0 would mean that the pixel is completly obscured.
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

#define D3DX_NORMALMAP_MIRROR_U     (1 << 16)
#define D3DX_NORMALMAP_MIRROR_V     (2 << 16)
#define D3DX_NORMALMAP_MIRROR       (3 << 16)
#define D3DX_NORMALMAP_INVERTSIGN   (8 << 16)
#define D3DX_NORMALMAP_COMPUTE_OCCLUSION (16 << 16)




//----------------------------------------------------------------------------
// D3DX_CHANNEL flags:
// -------------------
// These flags are used by functions which operate on or more channels
// in a texture.
//
// D3DX_CHANNEL_RED
//     Indicates the red channel should be used
// D3DX_CHANNEL_BLUE
//     Indicates the blue channel should be used
// D3DX_CHANNEL_GREEN
//     Indicates the green channel should be used
// D3DX_CHANNEL_ALPHA
//     Indicates the alpha channel should be used
// D3DX_CHANNEL_LUMINANCE
//     Indicates the luminaces of the red green and blue channels should be 
//     used.
//
//----------------------------------------------------------------------------

#define D3DX_CHANNEL_RED            (1 << 0)
#define D3DX_CHANNEL_BLUE           (1 << 1)
#define D3DX_CHANNEL_GREEN          (1 << 2)
#define D3DX_CHANNEL_ALPHA          (1 << 3)
#define D3DX_CHANNEL_LUMINANCE      (1 << 4)




//----------------------------------------------------------------------------
// D3DXIMAGE_FILEFORMAT:
// ---------------------
// This enum is used to describe supported image file formats.
//
//----------------------------------------------------------------------------

typedef enum _D3DXIMAGE_FILEFORMAT
{
    D3DXIFF_BMP         = 0,
    D3DXIFF_JPG         = 1,
    D3DXIFF_TGA         = 2,
    D3DXIFF_PNG         = 3,
    D3DXIFF_DDS         = 4,
    D3DXIFF_PPM         = 5,
    D3DXIFF_DIB         = 6,
    D3DXIFF_HDR         = 7,       //high dynamic range formats
    D3DXIFF_PFM         = 8,       //
    D3DXIFF_FORCE_DWORD = 0x7fffffff

} D3DXIMAGE_FILEFORMAT;


//----------------------------------------------------------------------------
// LPD3DXFILL2D and LPD3DXFILL3D:
// ------------------------------
// Function types used by the texture fill functions.
//
// Parameters:
//  pOut
//      Pointer to a vector which the function uses to return its result.
//      X,Y,Z,W will be mapped to R,G,B,A respectivly. 
//  pTexCoord
//      Pointer to a vector containing the coordinates of the texel currently 
//      being evaluated.  Textures and VolumeTexture texcoord components 
//      range from 0 to 1. CubeTexture texcoord component range from -1 to 1.
//  pTexelSize
//      Pointer to a vector containing the dimensions of the current texel.
//  pData
//      Pointer to user data.
//
//----------------------------------------------------------------------------

typedef VOID (WINAPI *LPD3DXFILL2D)(D3DXVECTOR4 *pOut, 
    CONST D3DXVECTOR2 *pTexCoord, CONST D3DXVECTOR2 *pTexelSize, LPVOID pData);

typedef VOID (WINAPI *LPD3DXFILL3D)(D3DXVECTOR4 *pOut, 
    CONST D3DXVECTOR3 *pTexCoord, CONST D3DXVECTOR3 *pTexelSize, LPVOID pData);
 


//----------------------------------------------------------------------------
// D3DXIMAGE_INFO:
// ---------------
// This structure is used to return a rough description of what the
// the original contents of an image file looked like.
// 
//  Width
//      Width of original image in pixels
//  Height
//      Height of original image in pixels
//  Depth
//      Depth of original image in pixels
//  MipLevels
//      Number of mip levels in original image
//  Format
//      D3D format which most closely describes the data in original image
//  ResourceType
//      D3DRESOURCETYPE representing the type of texture stored in the file.
//      D3DRTYPE_TEXTURE, D3DRTYPE_VOLUMETEXTURE, or D3DRTYPE_CUBETEXTURE.
//  ImageFileFormat
//      D3DXIMAGE_FILEFORMAT representing the format of the image file.
//
//----------------------------------------------------------------------------

typedef struct _D3DXIMAGE_INFO
{
    UINT                    Width;
    UINT                    Height;
    UINT                    Depth;
    UINT                    MipLevels;
    D3DFORMAT               Format;
    D3DRESOURCETYPE         ResourceType;
    D3DXIMAGE_FILEFORMAT    ImageFileFormat;

} D3DXIMAGE_INFO;





#ifdef __cplusplus
extern "C" {
#endif //__cplusplus



//////////////////////////////////////////////////////////////////////////////
// Image File APIs ///////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
;
//----------------------------------------------------------------------------
// GetImageInfoFromFile/Resource:
// ------------------------------
// Fills in a D3DXIMAGE_INFO struct with information about an image file.
//
// Parameters:
//  pSrcFile
//      File name of the source image.
//  pSrcModule
//      Module where resource is located, or NULL for module associated
//      with image the os used to create the current process.
//  pSrcResource
//      Resource name
//  pSrcData
//      Pointer to file in memory.
//  SrcDataSize
//      Size in bytes of file in memory.
//  pSrcInfo
//      Pointer to a D3DXIMAGE_INFO structure to be filled in with the 
//      description of the data in the source image file.
//
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXGetImageInfoFromFileA(
        LPCSTR                    pSrcFile,
        D3DXIMAGE_INFO*           pSrcInfo);

HRESULT WINAPI
    D3DXGetImageInfoFromFileW(
        LPCWSTR                   pSrcFile,
        D3DXIMAGE_INFO*           pSrcInfo);

#ifdef UNICODE
#define D3DXGetImageInfoFromFile D3DXGetImageInfoFromFileW
#else
#define D3DXGetImageInfoFromFile D3DXGetImageInfoFromFileA
#endif


HRESULT WINAPI
    D3DXGetImageInfoFromResourceA(
        HMODULE                   hSrcModule,
        LPCSTR                    pSrcResource,
        D3DXIMAGE_INFO*           pSrcInfo);

HRESULT WINAPI
    D3DXGetImageInfoFromResourceW(
        HMODULE                   hSrcModule,
        LPCWSTR                   pSrcResource,
        D3DXIMAGE_INFO*           pSrcInfo);

#ifdef UNICODE
#define D3DXGetImageInfoFromResource D3DXGetImageInfoFromResourceW
#else
#define D3DXGetImageInfoFromResource D3DXGetImageInfoFromResourceA
#endif


HRESULT WINAPI
    D3DXGetImageInfoFromFileInMemory(
        LPCVOID                   pSrcData,
        UINT                      SrcDataSize,
        D3DXIMAGE_INFO*           pSrcInfo);




//////////////////////////////////////////////////////////////////////////////
// Load/Save Surface APIs ////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// D3DXLoadSurfaceFromFile/Resource:
// ---------------------------------
// Load surface from a file or resource
//
// Parameters:
//  pDestSurface
//      Destination surface, which will receive the image.
//  pDestPalette
//      Destination palette of 256 colors, or NULL
//  pDestRect
//      Destination rectangle, or NULL for entire surface
//  pSrcFile
//      File name of the source image.
//  pSrcModule
//      Module where resource is located, or NULL for module associated
//      with image the os used to create the current process.
//  pSrcResource
//      Resource name
//  pSrcData
//      Pointer to file in memory.
//  SrcDataSize
//      Size in bytes of file in memory.
//  pSrcRect
//      Source rectangle, or NULL for entire image
//  Filter
//      D3DX_FILTER flags controlling how the image is filtered.
//      Or D3DX_DEFAULT for D3DX_FILTER_TRIANGLE.
//  ColorKey
//      Color to replace with transparent black, or 0 to disable colorkey.
//      This is always a 32-bit ARGB color, independent of the source image
//      format.  Alpha is significant, and should usually be set to FF for 
//      opaque colorkeys.  (ex. Opaque black == 0xff000000)
//  pSrcInfo
//      Pointer to a D3DXIMAGE_INFO structure to be filled in with the 
//      description of the data in the source image file, or NULL.
//
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXLoadSurfaceFromFileA(
        LPDIRECT3DSURFACE9        pDestSurface,
        CONST PALETTEENTRY*       pDestPalette,
        CONST RECT*               pDestRect,
        LPCSTR                    pSrcFile,
        CONST RECT*               pSrcRect,
        DWORD                     Filter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo);

HRESULT WINAPI
    D3DXLoadSurfaceFromFileW(
        LPDIRECT3DSURFACE9        pDestSurface,
        CONST PALETTEENTRY*       pDestPalette,
        CONST RECT*               pDestRect,
        LPCWSTR                   pSrcFile,
        CONST RECT*               pSrcRect,
        DWORD                     Filter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo);

#ifdef UNICODE
#define D3DXLoadSurfaceFromFile D3DXLoadSurfaceFromFileW
#else
#define D3DXLoadSurfaceFromFile D3DXLoadSurfaceFromFileA
#endif



HRESULT WINAPI
    D3DXLoadSurfaceFromResourceA(
        LPDIRECT3DSURFACE9        pDestSurface,
        CONST PALETTEENTRY*       pDestPalette,
        CONST RECT*               pDestRect,
        HMODULE                   hSrcModule,
        LPCSTR                    pSrcResource,
        CONST RECT*               pSrcRect,
        DWORD                     Filter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo);

HRESULT WINAPI
    D3DXLoadSurfaceFromResourceW(
        LPDIRECT3DSURFACE9        pDestSurface,
        CONST PALETTEENTRY*       pDestPalette,
        CONST RECT*               pDestRect,
        HMODULE                   hSrcModule,
        LPCWSTR                   pSrcResource,
        CONST RECT*               pSrcRect,
        DWORD                     Filter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo);


#ifdef UNICODE
#define D3DXLoadSurfaceFromResource D3DXLoadSurfaceFromResourceW
#else
#define D3DXLoadSurfaceFromResource D3DXLoadSurfaceFromResourceA
#endif



HRESULT WINAPI
    D3DXLoadSurfaceFromFileInMemory(
        LPDIRECT3DSURFACE9        pDestSurface,
        CONST PALETTEENTRY*       pDestPalette,
        CONST RECT*               pDestRect,
        LPCVOID                   pSrcData,
        UINT                      SrcDataSize,
        CONST RECT*               pSrcRect,
        DWORD                     Filter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo);



//----------------------------------------------------------------------------
// D3DXLoadSurfaceFromSurface:
// ---------------------------
// Load surface from another surface (with color conversion)
//
// Parameters:
//  pDestSurface
//      Destination surface, which will receive the image.
//  pDestPalette
//      Destination palette of 256 colors, or NULL
//  pDestRect
//      Destination rectangle, or NULL for entire surface
//  pSrcSurface
//      Source surface
//  pSrcPalette
//      Source palette of 256 colors, or NULL
//  pSrcRect
//      Source rectangle, or NULL for entire surface
//  Filter
//      D3DX_FILTER flags controlling how the image is filtered.
//      Or D3DX_DEFAULT for D3DX_FILTER_TRIANGLE.
//  ColorKey
//      Color to replace with transparent black, or 0 to disable colorkey.
//      This is always a 32-bit ARGB color, independent of the source image
//      format.  Alpha is significant, and should usually be set to FF for 
//      opaque colorkeys.  (ex. Opaque black == 0xff000000)
//
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXLoadSurfaceFromSurface(
        LPDIRECT3DSURFACE9        pDestSurface,
        CONST PALETTEENTRY*       pDestPalette,
        CONST RECT*               pDestRect,
        LPDIRECT3DSURFACE9        pSrcSurface,
        CONST PALETTEENTRY*       pSrcPalette,
        CONST RECT*               pSrcRect,
        DWORD                     Filter,
        D3DCOLOR                  ColorKey);


//----------------------------------------------------------------------------
// D3DXLoadSurfaceFromMemory:
// --------------------------
// Load surface from memory.
//
// Parameters:
//  pDestSurface
//      Destination surface, which will receive the image.
//  pDestPalette
//      Destination palette of 256 colors, or NULL
//  pDestRect
//      Destination rectangle, or NULL for entire surface
//  pSrcMemory
//      Pointer to the top-left corner of the source image in memory
//  SrcFormat
//      Pixel format of the source image.
//  SrcPitch
//      Pitch of source image, in bytes.  For DXT formats, this number
//      should represent the width of one row of cells, in bytes.
//  pSrcPalette
//      Source palette of 256 colors, or NULL
//  pSrcRect
//      Source rectangle.
//  Filter
//      D3DX_FILTER flags controlling how the image is filtered.
//      Or D3DX_DEFAULT for D3DX_FILTER_TRIANGLE.
//  ColorKey
//      Color to replace with transparent black, or 0 to disable colorkey.
//      This is always a 32-bit ARGB color, independent of the source image
//      format.  Alpha is significant, and should usually be set to FF for 
//      opaque colorkeys.  (ex. Opaque black == 0xff000000)
//
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXLoadSurfaceFromMemory(
        LPDIRECT3DSURFACE9        pDestSurface,
        CONST PALETTEENTRY*       pDestPalette,
        CONST RECT*               pDestRect,
        LPCVOID                   pSrcMemory,
        D3DFORMAT                 SrcFormat,
        UINT                      SrcPitch,
        CONST PALETTEENTRY*       pSrcPalette,
        CONST RECT*               pSrcRect,
        DWORD                     Filter,
        D3DCOLOR                  ColorKey);


//----------------------------------------------------------------------------
// D3DXSaveSurfaceToFile:
// ----------------------
// Save a surface to a image file.
//
// Parameters:
//  pDestFile
//      File name of the destination file
//  DestFormat
//      D3DXIMAGE_FILEFORMAT specifying file format to use when saving.
//  pSrcSurface
//      Source surface, containing the image to be saved
//  pSrcPalette
//      Source palette of 256 colors, or NULL
//  pSrcRect
//      Source rectangle, or NULL for the entire image
//
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXSaveSurfaceToFileA(
        LPCSTR                    pDestFile,
        D3DXIMAGE_FILEFORMAT      DestFormat,
        LPDIRECT3DSURFACE9        pSrcSurface,
        CONST PALETTEENTRY*       pSrcPalette,
        CONST RECT*               pSrcRect);

HRESULT WINAPI
    D3DXSaveSurfaceToFileW(
        LPCWSTR                   pDestFile,
        D3DXIMAGE_FILEFORMAT      DestFormat,
        LPDIRECT3DSURFACE9        pSrcSurface,
        CONST PALETTEENTRY*       pSrcPalette,
        CONST RECT*               pSrcRect);

#ifdef UNICODE
#define D3DXSaveSurfaceToFile D3DXSaveSurfaceToFileW
#else
#define D3DXSaveSurfaceToFile D3DXSaveSurfaceToFileA
#endif

//----------------------------------------------------------------------------
// D3DXSaveSurfaceToFileInMemory:
// ----------------------
// Save a surface to a image file.
//
// Parameters:
//  ppDestBuf
//      address of pointer to d3dxbuffer for returning data bits
//  DestFormat
//      D3DXIMAGE_FILEFORMAT specifying file format to use when saving.
//  pSrcSurface
//      Source surface, containing the image to be saved
//  pSrcPalette
//      Source palette of 256 colors, or NULL
//  pSrcRect
//      Source rectangle, or NULL for the entire image
//
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXSaveSurfaceToFileInMemory(
        LPD3DXBUFFER*             ppDestBuf,
        D3DXIMAGE_FILEFORMAT      DestFormat,
        LPDIRECT3DSURFACE9        pSrcSurface,
        CONST PALETTEENTRY*       pSrcPalette,
        CONST RECT*               pSrcRect);


//////////////////////////////////////////////////////////////////////////////
// Load/Save Volume APIs /////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// D3DXLoadVolumeFromFile/Resource:
// --------------------------------
// Load volume from a file or resource
//
// Parameters:
//  pDestVolume
//      Destination volume, which will receive the image.
//  pDestPalette
//      Destination palette of 256 colors, or NULL
//  pDestBox
//      Destination box, or NULL for entire volume
//  pSrcFile
//      File name of the source image.
//  pSrcModule
//      Module where resource is located, or NULL for module associated
//      with image the os used to create the current process.
//  pSrcResource
//      Resource name
//  pSrcData
//      Pointer to file in memory.
//  SrcDataSize
//      Size in bytes of file in memory.
//  pSrcBox
//      Source box, or NULL for entire image
//  Filter
//      D3DX_FILTER flags controlling how the image is filtered.
//      Or D3DX_DEFAULT for D3DX_FILTER_TRIANGLE.
//  ColorKey
//      Color to replace with transparent black, or 0 to disable colorkey.
//      This is always a 32-bit ARGB color, independent of the source image
//      format.  Alpha is significant, and should usually be set to FF for 
//      opaque colorkeys.  (ex. Opaque black == 0xff000000)
//  pSrcInfo
//      Pointer to a D3DXIMAGE_INFO structure to be filled in with the 
//      description of the data in the source image file, or NULL.
//
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXLoadVolumeFromFileA(
        LPDIRECT3DVOLUME9         pDestVolume,
        CONST PALETTEENTRY*       pDestPalette,
        CONST D3DBOX*             pDestBox,
        LPCSTR                    pSrcFile,
        CONST D3DBOX*             pSrcBox,
        DWORD                     Filter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo);

HRESULT WINAPI
    D3DXLoadVolumeFromFileW(
        LPDIRECT3DVOLUME9         pDestVolume,
        CONST PALETTEENTRY*       pDestPalette,
        CONST D3DBOX*             pDestBox,
        LPCWSTR                   pSrcFile,
        CONST D3DBOX*             pSrcBox,
        DWORD                     Filter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo);

#ifdef UNICODE
#define D3DXLoadVolumeFromFile D3DXLoadVolumeFromFileW
#else
#define D3DXLoadVolumeFromFile D3DXLoadVolumeFromFileA
#endif


HRESULT WINAPI
    D3DXLoadVolumeFromResourceA(
        LPDIRECT3DVOLUME9         pDestVolume,
        CONST PALETTEENTRY*       pDestPalette,
        CONST D3DBOX*             pDestBox,
        HMODULE                   hSrcModule,
        LPCSTR                    pSrcResource,
        CONST D3DBOX*             pSrcBox,
        DWORD                     Filter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo);

HRESULT WINAPI
    D3DXLoadVolumeFromResourceW(
        LPDIRECT3DVOLUME9         pDestVolume,
        CONST PALETTEENTRY*       pDestPalette,
        CONST D3DBOX*             pDestBox,
        HMODULE                   hSrcModule,
        LPCWSTR                   pSrcResource,
        CONST D3DBOX*             pSrcBox,
        DWORD                     Filter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo);

#ifdef UNICODE
#define D3DXLoadVolumeFromResource D3DXLoadVolumeFromResourceW
#else
#define D3DXLoadVolumeFromResource D3DXLoadVolumeFromResourceA
#endif



HRESULT WINAPI
    D3DXLoadVolumeFromFileInMemory(
        LPDIRECT3DVOLUME9         pDestVolume,
        CONST PALETTEENTRY*       pDestPalette,
        CONST D3DBOX*             pDestBox,
        LPCVOID                   pSrcData,
        UINT                      SrcDataSize,
        CONST D3DBOX*             pSrcBox,
        DWORD                     Filter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo);



//----------------------------------------------------------------------------
// D3DXLoadVolumeFromVolume:
// -------------------------
// Load volume from another volume (with color conversion)
//
// Parameters:
//  pDestVolume
//      Destination volume, which will receive the image.
//  pDestPalette
//      Destination palette of 256 colors, or NULL
//  pDestBox
//      Destination box, or NULL for entire volume
//  pSrcVolume
//      Source volume
//  pSrcPalette
//      Source palette of 256 colors, or NULL
//  pSrcBox
//      Source box, or NULL for entire volume
//  Filter
//      D3DX_FILTER flags controlling how the image is filtered.
//      Or D3DX_DEFAULT for D3DX_FILTER_TRIANGLE.
//  ColorKey
//      Color to replace with transparent black, or 0 to disable colorkey.
//      This is always a 32-bit ARGB color, independent of the source image
//      format.  Alpha is significant, and should usually be set to FF for 
//      opaque colorkeys.  (ex. Opaque black == 0xff000000)
//
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXLoadVolumeFromVolume(
        LPDIRECT3DVOLUME9         pDestVolume,
        CONST PALETTEENTRY*       pDestPalette,
        CONST D3DBOX*             pDestBox,
        LPDIRECT3DVOLUME9         pSrcVolume,
        CONST PALETTEENTRY*       pSrcPalette,
        CONST D3DBOX*             pSrcBox,
        DWORD                     Filter,
        D3DCOLOR                  ColorKey);



//----------------------------------------------------------------------------
// D3DXLoadVolumeFromMemory:
// -------------------------
// Load volume from memory.
//
// Parameters:
//  pDestVolume
//      Destination volume, which will receive the image.
//  pDestPalette
//      Destination palette of 256 colors, or NULL
//  pDestBox
//      Destination box, or NULL for entire volume
//  pSrcMemory
//      Pointer to the top-left corner of the source volume in memory
//  SrcFormat
//      Pixel format of the source volume.
//  SrcRowPitch
//      Pitch of source image, in bytes.  For DXT formats, this number
//      should represent the size of one row of cells, in bytes.
//  SrcSlicePitch
//      Pitch of source image, in bytes.  For DXT formats, this number
//      should represent the size of one slice of cells, in bytes.
//  pSrcPalette
//      Source palette of 256 colors, or NULL
//  pSrcBox
//      Source box.
//  Filter
//      D3DX_FILTER flags controlling how the image is filtered.
//      Or D3DX_DEFAULT for D3DX_FILTER_TRIANGLE.
//  ColorKey
//      Color to replace with transparent black, or 0 to disable colorkey.
//      This is always a 32-bit ARGB color, independent of the source image
//      format.  Alpha is significant, and should usually be set to FF for 
//      opaque colorkeys.  (ex. Opaque black == 0xff000000)
//
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXLoadVolumeFromMemory(
        LPDIRECT3DVOLUME9         pDestVolume,
        CONST PALETTEENTRY*       pDestPalette,
        CONST D3DBOX*             pDestBox,
        LPCVOID                   pSrcMemory,
        D3DFORMAT                 SrcFormat,
        UINT                      SrcRowPitch,
        UINT                      SrcSlicePitch,
        CONST PALETTEENTRY*       pSrcPalette,
        CONST D3DBOX*             pSrcBox,
        DWORD                     Filter,
        D3DCOLOR                  ColorKey);



//----------------------------------------------------------------------------
// D3DXSaveVolumeToFile:
// ---------------------
// Save a volume to a image file.
//
// Parameters:
//  pDestFile
//      File name of the destination file
//  DestFormat
//      D3DXIMAGE_FILEFORMAT specifying file format to use when saving.
//  pSrcVolume
//      Source volume, containing the image to be saved
//  pSrcPalette
//      Source palette of 256 colors, or NULL
//  pSrcBox
//      Source box, or NULL for the entire volume
//
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXSaveVolumeToFileA(
        LPCSTR                    pDestFile,
        D3DXIMAGE_FILEFORMAT      DestFormat,
        LPDIRECT3DVOLUME9         pSrcVolume,
        CONST PALETTEENTRY*       pSrcPalette,
        CONST D3DBOX*             pSrcBox);

HRESULT WINAPI
    D3DXSaveVolumeToFileW(
        LPCWSTR                   pDestFile,
        D3DXIMAGE_FILEFORMAT      DestFormat,
        LPDIRECT3DVOLUME9         pSrcVolume,
        CONST PALETTEENTRY*       pSrcPalette,
        CONST D3DBOX*             pSrcBox);

#ifdef UNICODE
#define D3DXSaveVolumeToFile D3DXSaveVolumeToFileW
#else
#define D3DXSaveVolumeToFile D3DXSaveVolumeToFileA
#endif


//----------------------------------------------------------------------------
// D3DXSaveVolumeToFileInMemory:
// ---------------------
// Save a volume to a image file.
//
// Parameters:
//  pDestFile
//      File name of the destination file
//  DestFormat
//      D3DXIMAGE_FILEFORMAT specifying file format to use when saving.
//  pSrcVolume
//      Source volume, containing the image to be saved
//  pSrcPalette
//      Source palette of 256 colors, or NULL
//  pSrcBox
//      Source box, or NULL for the entire volume
//
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXSaveVolumeToFileInMemory(
        LPD3DXBUFFER*             ppDestBuf,
        D3DXIMAGE_FILEFORMAT      DestFormat,
        LPDIRECT3DVOLUME9         pSrcVolume,
        CONST PALETTEENTRY*       pSrcPalette,
        CONST D3DBOX*             pSrcBox);

//////////////////////////////////////////////////////////////////////////////
// Create/Save Texture APIs //////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// D3DXCheckTextureRequirements:
// -----------------------------
// Checks texture creation parameters.  If parameters are invalid, this
// function returns corrected parameters.
//
// Parameters:
//
//  pDevice
//      The D3D device to be used
//  pWidth, pHeight, pDepth, pSize
//      Desired size in pixels, or NULL.  Returns corrected size.
//  pNumMipLevels
//      Number of desired mipmap levels, or NULL.  Returns corrected number.
//  Usage
//      Texture usage flags
//  pFormat
//      Desired pixel format, or NULL.  Returns corrected format.
//  Pool
//      Memory pool to be used to create texture
//
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXCheckTextureRequirements(
        LPDIRECT3DDEVICE9         pDevice,
        UINT*                     pWidth,
        UINT*                     pHeight,
        UINT*                     pNumMipLevels,
        DWORD                     Usage,
        D3DFORMAT*                pFormat,
        D3DPOOL                   Pool);

HRESULT WINAPI
    D3DXCheckCubeTextureRequirements(
        LPDIRECT3DDEVICE9         pDevice,
        UINT*                     pSize,
        UINT*                     pNumMipLevels,
        DWORD                     Usage,
        D3DFORMAT*                pFormat,
        D3DPOOL                   Pool);

HRESULT WINAPI
    D3DXCheckVolumeTextureRequirements(
        LPDIRECT3DDEVICE9         pDevice,
        UINT*                     pWidth,
        UINT*                     pHeight,
        UINT*                     pDepth,
        UINT*                     pNumMipLevels,
        DWORD                     Usage,
        D3DFORMAT*                pFormat,
        D3DPOOL                   Pool);


//----------------------------------------------------------------------------
// D3DXCreateTexture:
// ------------------
// Create an empty texture
//
// Parameters:
//
//  pDevice
//      The D3D device with which the texture is going to be used.
//  Width, Height, Depth, Size
//      size in pixels. these must be non-zero
//  MipLevels
//      number of mip levels desired. if zero or D3DX_DEFAULT, a complete
//      mipmap chain will be created.
//  Usage
//      Texture usage flags
//  Format
//      Pixel format.
//  Pool
//      Memory pool to be used to create texture
//  ppTexture, ppCubeTexture, ppVolumeTexture
//      The texture object that will be created
//
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXCreateTexture(
        LPDIRECT3DDEVICE9         pDevice,
        UINT                      Width,
        UINT                      Height,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        LPDIRECT3DTEXTURE9*       ppTexture);

HRESULT WINAPI
    D3DXCreateCubeTexture(
        LPDIRECT3DDEVICE9         pDevice,
        UINT                      Size,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        LPDIRECT3DCUBETEXTURE9*   ppCubeTexture);

HRESULT WINAPI
    D3DXCreateVolumeTexture(
        LPDIRECT3DDEVICE9         pDevice,
        UINT                      Width,
        UINT                      Height,
        UINT                      Depth,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        LPDIRECT3DVOLUMETEXTURE9* ppVolumeTexture);



//----------------------------------------------------------------------------
// D3DXCreateTextureFromFile/Resource:
// -----------------------------------
// Create a texture object from a file or resource.
//
// Parameters:
//
//  pDevice
//      The D3D device with which the texture is going to be used.
//  pSrcFile
//      File name.
//  hSrcModule
//      Module handle. if NULL, current module will be used.
//  pSrcResource
//      Resource name in module
//  pvSrcData
//      Pointer to file in memory.
//  SrcDataSize
//      Size in bytes of file in memory.
//  Width, Height, Depth, Size
//      Size in pixels.  If zero or D3DX_DEFAULT, the size will be taken from 
//      the file and rounded up to a power of two.  If D3DX_DEFAULT_NONPOW2, 
//      and the device supports NONPOW2 textures, the size will not be rounded.
//      If D3DX_FROM_FILE, the size will be taken exactly as it is in the file, 
//      and the call will fail if this violates device capabilities.
//  MipLevels
//      Number of mip levels.  If zero or D3DX_DEFAULT, a complete mipmap
//      chain will be created.  If D3DX_FROM_FILE, the size will be taken 
//      exactly as it is in the file, and the call will fail if this violates 
//      device capabilities.
//  Usage
//      Texture usage flags
//  Format
//      Desired pixel format.  If D3DFMT_UNKNOWN, the format will be
//      taken from the file.  If D3DFMT_FROM_FILE, the format will be taken
//      exactly as it is in the file, and the call will fail if the device does
//      not support the given format.
//  Pool
//      Memory pool to be used to create texture
//  Filter
//      D3DX_FILTER flags controlling how the image is filtered.
//      Or D3DX_DEFAULT for D3DX_FILTER_TRIANGLE.
//  MipFilter
//      D3DX_FILTER flags controlling how each miplevel is filtered.
//      Or D3DX_DEFAULT for D3DX_FILTER_BOX.
//      Use the D3DX_SKIP_DDS_MIP_LEVELS macro to specify both a filter and the
//      number of mip levels to skip when loading DDS files.
//  ColorKey
//      Color to replace with transparent black, or 0 to disable colorkey.
//      This is always a 32-bit ARGB color, independent of the source image
//      format.  Alpha is significant, and should usually be set to FF for 
//      opaque colorkeys.  (ex. Opaque black == 0xff000000)
//  pSrcInfo
//      Pointer to a D3DXIMAGE_INFO structure to be filled in with the 
//      description of the data in the source image file, or NULL.
//  pPalette
//      256 color palette to be filled in, or NULL
//  ppTexture, ppCubeTexture, ppVolumeTexture
//      The texture object that will be created
//
//----------------------------------------------------------------------------

// FromFile

HRESULT WINAPI
    D3DXCreateTextureFromFileA(
        LPDIRECT3DDEVICE9         pDevice,
        LPCSTR                    pSrcFile,
        LPDIRECT3DTEXTURE9*       ppTexture);

HRESULT WINAPI
    D3DXCreateTextureFromFileW(
        LPDIRECT3DDEVICE9         pDevice,
        LPCWSTR                   pSrcFile,
        LPDIRECT3DTEXTURE9*       ppTexture);

#ifdef UNICODE
#define D3DXCreateTextureFromFile D3DXCreateTextureFromFileW
#else
#define D3DXCreateTextureFromFile D3DXCreateTextureFromFileA
#endif


HRESULT WINAPI
    D3DXCreateCubeTextureFromFileA(
        LPDIRECT3DDEVICE9         pDevice,
        LPCSTR                    pSrcFile,
        LPDIRECT3DCUBETEXTURE9*   ppCubeTexture);

HRESULT WINAPI
    D3DXCreateCubeTextureFromFileW(
        LPDIRECT3DDEVICE9         pDevice,
        LPCWSTR                   pSrcFile,
        LPDIRECT3DCUBETEXTURE9*   ppCubeTexture);

#ifdef UNICODE
#define D3DXCreateCubeTextureFromFile D3DXCreateCubeTextureFromFileW
#else
#define D3DXCreateCubeTextureFromFile D3DXCreateCubeTextureFromFileA
#endif


HRESULT WINAPI
    D3DXCreateVolumeTextureFromFileA(
        LPDIRECT3DDEVICE9         pDevice,
        LPCSTR                    pSrcFile,
        LPDIRECT3DVOLUMETEXTURE9* ppVolumeTexture);

HRESULT WINAPI
    D3DXCreateVolumeTextureFromFileW(
        LPDIRECT3DDEVICE9         pDevice,
        LPCWSTR                   pSrcFile,
        LPDIRECT3DVOLUMETEXTURE9* ppVolumeTexture);

#ifdef UNICODE
#define D3DXCreateVolumeTextureFromFile D3DXCreateVolumeTextureFromFileW
#else
#define D3DXCreateVolumeTextureFromFile D3DXCreateVolumeTextureFromFileA
#endif


// FromResource

HRESULT WINAPI
    D3DXCreateTextureFromResourceA(
        LPDIRECT3DDEVICE9         pDevice,
        HMODULE                   hSrcModule,
        LPCSTR                    pSrcResource,
        LPDIRECT3DTEXTURE9*       ppTexture);

HRESULT WINAPI
    D3DXCreateTextureFromResourceW(
        LPDIRECT3DDEVICE9         pDevice,
        HMODULE                   hSrcModule,
        LPCWSTR                   pSrcResource,
        LPDIRECT3DTEXTURE9*       ppTexture);

#ifdef UNICODE
#define D3DXCreateTextureFromResource D3DXCreateTextureFromResourceW
#else
#define D3DXCreateTextureFromResource D3DXCreateTextureFromResourceA
#endif


HRESULT WINAPI
    D3DXCreateCubeTextureFromResourceA(
        LPDIRECT3DDEVICE9         pDevice,
        HMODULE                   hSrcModule,
        LPCSTR                    pSrcResource,
        LPDIRECT3DCUBETEXTURE9*   ppCubeTexture);

HRESULT WINAPI
    D3DXCreateCubeTextureFromResourceW(
        LPDIRECT3DDEVICE9         pDevice,
        HMODULE                   hSrcModule,
        LPCWSTR                   pSrcResource,
        LPDIRECT3DCUBETEXTURE9*   ppCubeTexture);

#ifdef UNICODE
#define D3DXCreateCubeTextureFromResource D3DXCreateCubeTextureFromResourceW
#else
#define D3DXCreateCubeTextureFromResource D3DXCreateCubeTextureFromResourceA
#endif


HRESULT WINAPI
    D3DXCreateVolumeTextureFromResourceA(
        LPDIRECT3DDEVICE9         pDevice,
        HMODULE                   hSrcModule,
        LPCSTR                    pSrcResource,
        LPDIRECT3DVOLUMETEXTURE9* ppVolumeTexture);

HRESULT WINAPI
    D3DXCreateVolumeTextureFromResourceW(
        LPDIRECT3DDEVICE9         pDevice,
        HMODULE                   hSrcModule,
        LPCWSTR                   pSrcResource,
        LPDIRECT3DVOLUMETEXTURE9* ppVolumeTexture);

#ifdef UNICODE
#define D3DXCreateVolumeTextureFromResource D3DXCreateVolumeTextureFromResourceW
#else
#define D3DXCreateVolumeTextureFromResource D3DXCreateVolumeTextureFromResourceA
#endif


// FromFileEx

HRESULT WINAPI
    D3DXCreateTextureFromFileExA(
        LPDIRECT3DDEVICE9         pDevice,
        LPCSTR                    pSrcFile,
        UINT                      Width,
        UINT                      Height,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        DWORD                     Filter,
        DWORD                     MipFilter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo,
        PALETTEENTRY*             pPalette,
        LPDIRECT3DTEXTURE9*       ppTexture);

HRESULT WINAPI
    D3DXCreateTextureFromFileExW(
        LPDIRECT3DDEVICE9         pDevice,
        LPCWSTR                   pSrcFile,
        UINT                      Width,
        UINT                      Height,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        DWORD                     Filter,
        DWORD                     MipFilter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo,
        PALETTEENTRY*             pPalette,
        LPDIRECT3DTEXTURE9*       ppTexture);

#ifdef UNICODE
#define D3DXCreateTextureFromFileEx D3DXCreateTextureFromFileExW
#else
#define D3DXCreateTextureFromFileEx D3DXCreateTextureFromFileExA
#endif


HRESULT WINAPI
    D3DXCreateCubeTextureFromFileExA(
        LPDIRECT3DDEVICE9         pDevice,
        LPCSTR                    pSrcFile,
        UINT                      Size,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        DWORD                     Filter,
        DWORD                     MipFilter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo,
        PALETTEENTRY*             pPalette,
        LPDIRECT3DCUBETEXTURE9*   ppCubeTexture);

HRESULT WINAPI
    D3DXCreateCubeTextureFromFileExW(
        LPDIRECT3DDEVICE9         pDevice,
        LPCWSTR                   pSrcFile,
        UINT                      Size,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        DWORD                     Filter,
        DWORD                     MipFilter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo,
        PALETTEENTRY*             pPalette,
        LPDIRECT3DCUBETEXTURE9*   ppCubeTexture);

#ifdef UNICODE
#define D3DXCreateCubeTextureFromFileEx D3DXCreateCubeTextureFromFileExW
#else
#define D3DXCreateCubeTextureFromFileEx D3DXCreateCubeTextureFromFileExA
#endif


HRESULT WINAPI
    D3DXCreateVolumeTextureFromFileExA(
        LPDIRECT3DDEVICE9         pDevice,
        LPCSTR                    pSrcFile,
        UINT                      Width,
        UINT                      Height,
        UINT                      Depth,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        DWORD                     Filter,
        DWORD                     MipFilter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo,
        PALETTEENTRY*             pPalette,
        LPDIRECT3DVOLUMETEXTURE9* ppVolumeTexture);

HRESULT WINAPI
    D3DXCreateVolumeTextureFromFileExW(
        LPDIRECT3DDEVICE9         pDevice,
        LPCWSTR                   pSrcFile,
        UINT                      Width,
        UINT                      Height,
        UINT                      Depth,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        DWORD                     Filter,
        DWORD                     MipFilter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo,
        PALETTEENTRY*             pPalette,
        LPDIRECT3DVOLUMETEXTURE9* ppVolumeTexture);

#ifdef UNICODE
#define D3DXCreateVolumeTextureFromFileEx D3DXCreateVolumeTextureFromFileExW
#else
#define D3DXCreateVolumeTextureFromFileEx D3DXCreateVolumeTextureFromFileExA
#endif


// FromResourceEx

HRESULT WINAPI
    D3DXCreateTextureFromResourceExA(
        LPDIRECT3DDEVICE9         pDevice,
        HMODULE                   hSrcModule,
        LPCSTR                    pSrcResource,
        UINT                      Width,
        UINT                      Height,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        DWORD                     Filter,
        DWORD                     MipFilter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo,
        PALETTEENTRY*             pPalette,
        LPDIRECT3DTEXTURE9*       ppTexture);

HRESULT WINAPI
    D3DXCreateTextureFromResourceExW(
        LPDIRECT3DDEVICE9         pDevice,
        HMODULE                   hSrcModule,
        LPCWSTR                   pSrcResource,
        UINT                      Width,
        UINT                      Height,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        DWORD                     Filter,
        DWORD                     MipFilter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo,
        PALETTEENTRY*             pPalette,
        LPDIRECT3DTEXTURE9*       ppTexture);

#ifdef UNICODE
#define D3DXCreateTextureFromResourceEx D3DXCreateTextureFromResourceExW
#else
#define D3DXCreateTextureFromResourceEx D3DXCreateTextureFromResourceExA
#endif


HRESULT WINAPI
    D3DXCreateCubeTextureFromResourceExA(
        LPDIRECT3DDEVICE9         pDevice,
        HMODULE                   hSrcModule,
        LPCSTR                    pSrcResource,
        UINT                      Size,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        DWORD                     Filter,
        DWORD                     MipFilter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo,
        PALETTEENTRY*             pPalette,
        LPDIRECT3DCUBETEXTURE9*   ppCubeTexture);

HRESULT WINAPI
    D3DXCreateCubeTextureFromResourceExW(
        LPDIRECT3DDEVICE9         pDevice,
        HMODULE                   hSrcModule,
        LPCWSTR                   pSrcResource,
        UINT                      Size,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        DWORD                     Filter,
        DWORD                     MipFilter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo,
        PALETTEENTRY*             pPalette,
        LPDIRECT3DCUBETEXTURE9*   ppCubeTexture);

#ifdef UNICODE
#define D3DXCreateCubeTextureFromResourceEx D3DXCreateCubeTextureFromResourceExW
#else
#define D3DXCreateCubeTextureFromResourceEx D3DXCreateCubeTextureFromResourceExA
#endif


HRESULT WINAPI
    D3DXCreateVolumeTextureFromResourceExA(
        LPDIRECT3DDEVICE9         pDevice,
        HMODULE                   hSrcModule,
        LPCSTR                    pSrcResource,
        UINT                      Width,
        UINT                      Height,
        UINT                      Depth,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        DWORD                     Filter,
        DWORD                     MipFilter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo,
        PALETTEENTRY*             pPalette,
        LPDIRECT3DVOLUMETEXTURE9* ppVolumeTexture);

HRESULT WINAPI
    D3DXCreateVolumeTextureFromResourceExW(
        LPDIRECT3DDEVICE9         pDevice,
        HMODULE                   hSrcModule,
        LPCWSTR                   pSrcResource,
        UINT                      Width,
        UINT                      Height,
        UINT                      Depth,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        DWORD                     Filter,
        DWORD                     MipFilter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo,
        PALETTEENTRY*             pPalette,
        LPDIRECT3DVOLUMETEXTURE9* ppVolumeTexture);

#ifdef UNICODE
#define D3DXCreateVolumeTextureFromResourceEx D3DXCreateVolumeTextureFromResourceExW
#else
#define D3DXCreateVolumeTextureFromResourceEx D3DXCreateVolumeTextureFromResourceExA
#endif


// FromFileInMemory

HRESULT WINAPI
    D3DXCreateTextureFromFileInMemory(
        LPDIRECT3DDEVICE9         pDevice,
        LPCVOID                   pSrcData,
        UINT                      SrcDataSize,
        LPDIRECT3DTEXTURE9*       ppTexture);

HRESULT WINAPI
    D3DXCreateCubeTextureFromFileInMemory(
        LPDIRECT3DDEVICE9         pDevice,
        LPCVOID                   pSrcData,
        UINT                      SrcDataSize,
        LPDIRECT3DCUBETEXTURE9*   ppCubeTexture);

HRESULT WINAPI
    D3DXCreateVolumeTextureFromFileInMemory(
        LPDIRECT3DDEVICE9         pDevice,
        LPCVOID                   pSrcData,
        UINT                      SrcDataSize,
        LPDIRECT3DVOLUMETEXTURE9* ppVolumeTexture);


// FromFileInMemoryEx

HRESULT WINAPI
    D3DXCreateTextureFromFileInMemoryEx(
        LPDIRECT3DDEVICE9         pDevice,
        LPCVOID                   pSrcData,
        UINT                      SrcDataSize,
        UINT                      Width,
        UINT                      Height,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        DWORD                     Filter,
        DWORD                     MipFilter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo,
        PALETTEENTRY*             pPalette,
        LPDIRECT3DTEXTURE9*       ppTexture);

HRESULT WINAPI
    D3DXCreateCubeTextureFromFileInMemoryEx(
        LPDIRECT3DDEVICE9         pDevice,
        LPCVOID                   pSrcData,
        UINT                      SrcDataSize,
        UINT                      Size,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        DWORD                     Filter,
        DWORD                     MipFilter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo,
        PALETTEENTRY*             pPalette,
        LPDIRECT3DCUBETEXTURE9*   ppCubeTexture);

HRESULT WINAPI
    D3DXCreateVolumeTextureFromFileInMemoryEx(
        LPDIRECT3DDEVICE9         pDevice,
        LPCVOID                   pSrcData,
        UINT                      SrcDataSize,
        UINT                      Width,
        UINT                      Height,
        UINT                      Depth,
        UINT                      MipLevels,
        DWORD                     Usage,
        D3DFORMAT                 Format,
        D3DPOOL                   Pool,
        DWORD                     Filter,
        DWORD                     MipFilter,
        D3DCOLOR                  ColorKey,
        D3DXIMAGE_INFO*           pSrcInfo,
        PALETTEENTRY*             pPalette,
        LPDIRECT3DVOLUMETEXTURE9* ppVolumeTexture);



//----------------------------------------------------------------------------
// D3DXSaveTextureToFile:
// ----------------------
// Save a texture to a file.
//
// Parameters:
//  pDestFile
//      File name of the destination file
//  DestFormat
//      D3DXIMAGE_FILEFORMAT specifying file format to use when saving.
//  pSrcTexture
//      Source texture, containing the image to be saved
//  pSrcPalette
//      Source palette of 256 colors, or NULL
//
//----------------------------------------------------------------------------


HRESULT WINAPI
    D3DXSaveTextureToFileA(
        LPCSTR                    pDestFile,
        D3DXIMAGE_FILEFORMAT      DestFormat,
        LPDIRECT3DBASETEXTURE9    pSrcTexture,
        CONST PALETTEENTRY*       pSrcPalette);

HRESULT WINAPI
    D3DXSaveTextureToFileW(
        LPCWSTR                   pDestFile,
        D3DXIMAGE_FILEFORMAT      DestFormat,
        LPDIRECT3DBASETEXTURE9    pSrcTexture,
        CONST PALETTEENTRY*       pSrcPalette);

#ifdef UNICODE
#define D3DXSaveTextureToFile D3DXSaveTextureToFileW
#else
#define D3DXSaveTextureToFile D3DXSaveTextureToFileA
#endif


//----------------------------------------------------------------------------
// D3DXSaveTextureToFileInMemory:
// ----------------------
// Save a texture to a file.
//
// Parameters:
//  ppDestBuf
//      address of a d3dxbuffer pointer to return the image data
//  DestFormat
//      D3DXIMAGE_FILEFORMAT specifying file format to use when saving.
//  pSrcTexture
//      Source texture, containing the image to be saved
//  pSrcPalette
//      Source palette of 256 colors, or NULL
//
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXSaveTextureToFileInMemory(
        LPD3DXBUFFER*             ppDestBuf,
        D3DXIMAGE_FILEFORMAT      DestFormat,
        LPDIRECT3DBASETEXTURE9    pSrcTexture,
        CONST PALETTEENTRY*       pSrcPalette);




//////////////////////////////////////////////////////////////////////////////
// Misc Texture APIs /////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// D3DXFilterTexture:
// ------------------
// Filters mipmaps levels of a texture.
//
// Parameters:
//  pBaseTexture
//      The texture object to be filtered
//  pPalette
//      256 color palette to be used, or NULL for non-palettized formats
//  SrcLevel
//      The level whose image is used to generate the subsequent levels. 
//  Filter
//      D3DX_FILTER flags controlling how each miplevel is filtered.
//      Or D3DX_DEFAULT for D3DX_FILTER_BOX,
//
//----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXFilterTexture(
        LPDIRECT3DBASETEXTURE9    pBaseTexture,
        CONST PALETTEENTRY*       pPalette,
        UINT                      SrcLevel,
        DWORD                     Filter);

#define D3DXFilterCubeTexture D3DXFilterTexture
#define D3DXFilterVolumeTexture D3DXFilterTexture



//----------------------------------------------------------------------------
// D3DXFillTexture:
// ----------------
// Uses a user provided function to fill each texel of each mip level of a
// given texture.
//
// Paramters:
//  pTexture, pCubeTexture, pVolumeTexture
//      Pointer to the texture to be filled.
//  pFunction
//      Pointer to user provided evalutor function which will be used to 
//      compute the value of each texel.
//  pData
//      Pointer to an arbitrary block of user defined data.  This pointer 
//      will be passed to the function provided in pFunction
//-----------------------------------------------------------------------------

HRESULT WINAPI
    D3DXFillTexture(
        LPDIRECT3DTEXTURE9        pTexture,
        LPD3DXFILL2D              pFunction,
        LPVOID                    pData);

HRESULT WINAPI
    D3DXFillCubeTexture(
        LPDIRECT3DCUBETEXTURE9    pCubeTexture,
        LPD3DXFILL3D              pFunction,
        LPVOID                    pData);

HRESULT WINAPI
    D3DXFillVolumeTexture(
        LPDIRECT3DVOLUMETEXTURE9  pVolumeTexture,
        LPD3DXFILL3D              pFunction,
        LPVOID                    pData);

//---------------------------------------------------------------------------
// D3DXFillTextureTX:
// ------------------
// Uses a TX Shader target to function to fill each texel of each mip level
// of a given texture. The TX Shader target should be a compiled function 
// taking 2 paramters and returning a float4 color.
//
// Paramters:
//  pTexture, pCubeTexture, pVolumeTexture
//      Pointer to the texture to be filled.
//  pTextureShader
//      Pointer to the texture shader to be used to fill in the texture
//----------------------------------------------------------------------------

HRESULT WINAPI 
    D3DXFillTextureTX(
        LPDIRECT3DTEXTURE9        pTexture,
        LPD3DXTEXTURESHADER       pTextureShader);


HRESULT WINAPI
    D3DXFillCubeTextureTX(
        LPDIRECT3DCUBETEXTURE9    pCubeTexture,
        LPD3DXTEXTURESHADER       pTextureShader);
                                                
                                                        
HRESULT WINAPI 
    D3DXFillVolumeTextureTX(
        LPDIRECT3DVOLUMETEXTURE9  pVolumeTexture,
        LPD3DXTEXTURESHADER       pTextureShader);



//----------------------------------------------------------------------------
// D3DXComputeNormalMap:
// ---------------------
// Converts a height map into a normal map.  The (x,y,z) components of each
// normal are mapped to the (r,g,b) channels of the output texture.
//
// Parameters
//  pTexture
//      Pointer to the destination texture
//  pSrcTexture
//      Pointer to the source heightmap texture 
//  pSrcPalette
//      Source palette of 256 colors, or NULL
//  Flags
//      D3DX_NORMALMAP flags
//  Channel
//      D3DX_CHANNEL specifying source of height information
//  Amplitude
//      The constant value which the height information is multiplied by.
//---------------------------------------------------------------------------

HRESULT WINAPI
    D3DXComputeNormalMap(
        LPDIRECT3DTEXTURE9        pTexture,
        LPDIRECT3DTEXTURE9        pSrcTexture,
        CONST PALETTEENTRY*       pSrcPalette,
        DWORD                     Flags,
        DWORD                     Channel,
        FLOAT                     Amplitude);




#ifdef __cplusplus
}
#endif //__cplusplus

#endif //__D3DX9TEX_H__
#pragma endregion

#include <DirectXTex/DirectXTex.h>

#include <glm/detail/setup.hpp>
#include <glm/gtc/type_precision.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/packing.hpp>

extern bool __SK_bypass;

#ifndef _A2
#define _A2(a)     #a
#define  _A(a)  _A2(a)
#define _L2(w)  L ## w
#define  _L(w)   _L2(w)
#endif

extern const wchar_t* SK_VersionStrW;
extern const char*    SK_VersionStrA;

#ifndef __cpp_lib_format
#define __cpp_lib_format
#endif

#include <format>

#pragma warning ( disable : 4652 )

#define RESHADE_NO_IMGUI