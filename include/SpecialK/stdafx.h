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

#pragma warning ( disable : 4652 )

#pragma once

#ifdef _MSC_VER
__pragma (runtime_checks ("", off))
#endif

//#define _CRT_NON_CONFORMING_WCSTOK
//#define WIN32_LEAN_AND_MEAN

#include "targetver.h"

#define UNICODE 1

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

#include <../depends/include/glm/glm.hpp>
#include <d3dcompiler.h>
#include <d3d11shader.h>
#include <DirectXMath.h>
#include <DirectXMathConvert.inl>
#include <DirectXMathVector.inl>
#include <DirectXMathMatrix.inl>
#include <DirectXMathMisc.inl>

#include <LibLoaderAPI.h>

// Fix warnings in dbghelp.h
#pragma warning   (push)
#pragma warning   (disable: 4091)
#pragma warning   (disable: 4996)
#pragma warning   (disable: 4706)
#define _IMAGEHLP_SOURCE_
#  include <DbgHelp.h>
#pragma warning   (pop)

#include <CEGUI/CEGUI.h>
#include <CEGUI/System.h>

#include <concurrent_queue.h>
#include <concurrent_vector.h>
#include <concurrent_unordered_set.h>
#include <concurrent_unordered_map.h>
#include <concurrent_priority_queue.h>

#include <SpecialK/SpecialK.h>

#include <SpecialK/injection/injection.h>
#include <SpecialK/plugin/reshade.h>

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
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

#include <dxgi1_3.h>
#include <d3d11_2.h>

#include <d3dx9.h>
#include <d3dx9core.h>
#include <d3dx9math.h>
#include <d3dx9mesh.h>
#include <d3dx9shader.h>
#include <d3dx9effect.h>
#include <d3dx9tex.h>
#include <d3dx9core.h>
#include <d3dx9shape.h>

#include <DirectXTex/DirectXTex.h>

#include <glm/detail/setup.hpp>
#include <glm/gtc/type_precision.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/packing.hpp>

extern bool __SK_bypass;

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

#pragma warning ( disable : 4652 )