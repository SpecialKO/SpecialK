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

#define _CRT_NON_CONFORMING_WCSTOK
#define WIN32_LEAN_AND_MEAN

#include "targetver.h"

#ifndef WINVER
#define WINVER 	_WIN32_WINNT_WIN7
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 	_WIN32_WINNT_WIN7
#endif

#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x0501
#endif

#ifndef _WIN32_IE
#define _WIN32_IE _WIN32_IE_WIN7
#endif

#define UNICODE 1

#include <SpecialK/DLL_VERSION.H>

#include <Windows.h>
#include <windowsx.h>

#include <wingdi.h>
#include <atldef.h>
#include <atliface.h>
#include <atlbase.h>
#include <intsafe.h>
#include <strsafe.h>
#include <comdef.h>
#include <combaseapi.h>

#include <vcruntime_exception.h>

#include <gsl/gsl>
#include <gsl/span>
#include <gsl/pointers>
#include <gsl/gsl_util>
#include <gsl/string_span>

#include <eh.h>
#include <io.h>

#include <SpecialK/hash.h>
#include <SpecialK/crc32.h>
#include <SpecialK/utility.h>

#include <ctime>
#include <cfloat>
#include <limits>
#include <numeric>
#include <functional>
#include <algorithm>
#include <typeindex>

#include <string>
#include <sstream>

#include <map>
#include <set>
#include <stack>
#include <queue>
#include <array>
#include <vector>
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

#include <SpecialK/../../depends/include/glm/glm.hpp>

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

#include <SpecialK/SpecialK.h>

#include <SpecialK/injection/injection.h>
#include <SpecialK/plugin/reshade.h>

#include <dxgi.h>
#include <d3d11.h>

#include <SpecialK/render/backend.h>
#include <SpecialK/render/dxgi/dxgi_hdr.h>
#include <SpecialK/render/dxgi/dxgi_backend.h>
#include <SpecialK/render/dxgi/dxgi_swapchain.h>
#include <SpecialK/render/dxgi/dxgi_interfaces.h>
#include <SpecialK/render/d3d11/d3d11_state_tracker.h>
#include <SpecialK/render/d3d11/d3d11_4.h>

#include <SpecialK/render/vk/vulkan_backend.h>
#include <SpecialK/render/gl/opengl_backend.h>

#include <SpecialK/render/screenshot.h>

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#include <imgui/imgui.h>
#include <imgui/backends/imgui_d3d11.h>

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
#pragma comment (lib,    "winmm.lib")
#pragma comment (lib,    "secur32.lib")
#pragma comment (lib,    "wininet.lib")
#pragma comment (lib,    "Shlwapi.lib")
#pragma comment (lib,    "comctl32.lib")
#pragma comment (lib,    "delayimp.lib")
#pragma comment (lib,    "wbemuuid.lib")
#pragma comment (lib,    "PowrProf.lib")
#pragma comment (linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' "  \
                         "version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df'" \
                         " language='*'\"")

#include <SpecialK/update/version.h>
#include <SpecialK/update/archive.h>
#include <SpecialK/update/network.h>


#include <SpecialK/import.h>

#include <SpecialK/ansel.h>

#include <SpecialK/resource.h>
#include <SpecialK/com_util.h>

#include <dinput.h>
#include <SpecialK/input/input.h>
#include <SpecialK/input/steam.h>
#include <SpecialK/input/xinput.h>
#include <SpecialK/input/xinput_hotplug.h>
#include <SpecialK/input/dinput7_backend.h>
#include <SpecialK/input/dinput8_backend.h>

#include <SpecialK/osd/text.h>

#include <SpecialK/core.h>
#include <SpecialK/sound.h>
#include <SpecialK/console.h>
#include <SpecialK/command.h>
#include <SpecialK/parameter.h>

#include <SpecialK/tls.h>
#include <SpecialK/log.h>
#include <SpecialK/ini.h>
#include <SpecialK/hooks.h>
#include <SpecialK/config.h>
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
#include <SpecialK/performance/memory_monitor.h>

extern bool __SK_bypass;

#pragma warning ( disable : 4652 )