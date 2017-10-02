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

#include <SpecialK/vulkan_backend.h>
#include <SpecialK/log.h>
#include <SpecialK/utility.h>
#include <SpecialK/core.h>

#include <SpecialK/config.h>
#include <SpecialK/hooks.h>

#include <SpecialK/framerate.h>
#include <SpecialK/diagnostics/compatibility.h>

#include <Shlwapi.h>
#include <process.h>

volatile LONG __vk_ready = FALSE;

void
WaitForInit_Vk (void)
{
  while (! ReadAcquire (&__vk_ready))
    MsgWaitForMultipleObjectsEx (0, nullptr, config.system.init_delay, QS_ALLINPUT, MWMO_ALERTABLE);
}

extern HMODULE WINAPI SK_GetDLL   (void);
extern void           WaitForInit (void);
extern bool           SK_InitCOM  (void);

const wchar_t*
__stdcall
SK_DescribeVkResult (VkResult result)
{
  switch (result)
  {
    case VK_SUCCESS:
      return L"VK_SUCCESS";

    case VK_NOT_READY:
      return L"VK_NOT_READY";
    case VK_TIMEOUT:
      return L"VK_TIMEOUT";
    case VK_EVENT_SET:
      return L"VK_EVENT_SET";
    case VK_EVENT_RESET:
      return L"VK_EVENT_RESET";
    case VK_INCOMPLETE:
      return L"VK_INCOMPLETE";

    case VK_ERROR_OUT_OF_HOST_MEMORY:
      return L"VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
      return L"VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED:
      return L"VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST:
      return L"VK_ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED:
      return L"VK_ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT:
      return L"VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT:
      return L"VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT:
      return L"VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER:
      return L"VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_TOO_MANY_OBJECTS:
      return L"VK_ERROR_TOO_MANY_OBJECTS";
    case VK_ERROR_FORMAT_NOT_SUPPORTED:
      return L"VK_ERROR_FORMAT_NOT_SUPPORTED";
    case VK_ERROR_FRAGMENTED_POOL:
      return L"VK_ERROR_FRAGMENTED_POOL";
    case VK_ERROR_SURFACE_LOST_KHR:
      return L"VK_ERROR_SURFACE_LOST_KHR";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
      return L"VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";

    case VK_SUBOPTIMAL_KHR:
      return L"VK_SUBOPTIMAL_KHR";

    case VK_ERROR_OUT_OF_DATE_KHR:
      return L"VK_ERROR_OUT_OF_DATE_KHR";
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
      return L"VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
    case VK_ERROR_VALIDATION_FAILED_EXT:
      return L"VK_ERROR_VALIDATION_FAILED_EXT";
    case VK_ERROR_INVALID_SHADER_NV:
      return L"VK_ERROR_INVALID_SHADER_NV";

  default:
    dll_log.Log (L" *** Encountered unknown VkResult: (0x%08X)",
      static_cast <unsigned long> (result));
    return L"UNKNOWN";
  }
}


extern HWND hWndRender;

#define VK_CALL(_Ret, _Call) {                                        \
  (_Ret) = (_Call);                                                   \
  dll_log.Log ( L"[  Vulkan  ] [@]  Return: %s  -  < " L#_Call L" >", \
                SK_DescribeVkResult (_Ret) );                         \
}

#define VK_LOG_CALL(_Name,_Format)                        \
  dll_log.LogEx (true, L"[  Vulkan  ] [!] %s (", _Name);  \
  dll_log.LogEx (false, _Format

#define VK_LOG_CALL_END                                   \
  dll_log.LogEx (false, L") -- [%s, tid=0x%04x]\n",       \
    SK_GetCallerName ().c_str (), GetCurrentThreadId ());

VkResult
VKAPI_CALL
vkCreateWin32SurfaceKHR_Detour ( VkInstance                   instance,
                           const VkWin32SurfaceCreateInfoKHR* pCreateInfo,
                           const VkAllocationCallbacks*       pAllocator,
                                 VkSurfaceKHR*                pSurface )
{
  VK_LOG_CALL (L" [!] vkCreateWin32SurfaceKHR", L"HWND=%p"), pCreateInfo->hwnd);
  VK_LOG_CALL_END;

  VkResult ret;

  if ((hWndRender == 0 || (! IsWindow (hWndRender)) || (GetForegroundWindow () != hWndRender)) && IsWindow (pCreateInfo->hwnd)) {
    hWndRender = pCreateInfo->hwnd;

    // Trigger certain events that only happen after a few frames are drawn
    //
    //   I haven't hooked queue creation yet and installed fences in order to
    //     actually measure framerate in games such as DOOM, that do not use
    //       traditional swapchain presentation.
    for (int i = 0; i < 30; i++) {
      SK_BeginBufferSwap ();
      SK_EndBufferSwap   (S_OK);
    }
  }

  VK_CALL ( ret, SK::Vulkan::funcs.vkCreateWin32SurfaceKHR (
                   instance,
                     pCreateInfo,
                       pAllocator,
                         pSurface
                 )
          );

  return ret;
}

VkResult
VKAPI_CALL
vkCreateSwapchainKHR_Detour ( VkDevice                  device,
                        const VkSwapchainCreateInfoKHR* pCreateInfo,
                        const VkAllocationCallbacks*    pAllocator,
                              VkSwapchainKHR*           pSwapchain )
{
  VK_LOG_CALL (L" [!] vkCreateSwapchainKHR", L"..."));
  VK_LOG_CALL_END;

  VkResult ret;

  VK_CALL ( ret, SK::Vulkan::funcs.vkCreateSwapchainKHR (
                   device,
                     pCreateInfo,
                       pAllocator,
                         pSwapchain
                 )
          );

  return ret;
}

VkResult
VKAPI_CALL
vkQueuePresentKHR_Detour ( VkQueue           queue,
                     const VkPresentInfoKHR* pPresentInfo )
{
  SK_BeginBufferSwap ();

  VkResult ret =
    SK::Vulkan::funcs.vkQueuePresentKHR ( queue,
                                            pPresentInfo );

  SK_EndBufferSwap (S_OK);

  return ret;
}

VkResult
VKAPI_CALL
vkAcquireNextImageKHR_Detour ( VkDevice       device,
                               VkSwapchainKHR swapchain,
                               uint64_t       timeout,
                               VkSemaphore    semaphore,
                               VkFence        fence,
                               uint32_t*      pImageIndex )
{
  SK_BeginBufferSwap ();

  VkResult ret =
    SK::Vulkan::funcs.vkAcquireNextImageKHR ( device,
                                                swapchain,
                                                  timeout,
                                                    semaphore,
                                                      fence,
                                                        pImageIndex );

  SK_EndBufferSwap (S_OK);

  return ret;
}

extern void SK_BootVulkan (void);
typedef void (WINAPI *finish_pfn)(void);

void
WINAPI
vulkan_init_callback (finish_pfn finish)
{
  SK_BootVulkan ();

  finish ();
}

void
WINAPI
SK_HookVulkan (void)
{
#ifdef _WIN64
  static volatile ULONG hooked = FALSE;

  if (InterlockedCompareExchange (&hooked, TRUE, FALSE))
    return;

  if (! config.apis.Vulkan.hook)
    return;

  if (! StrStrIW ( SK_GetModuleName (SK_GetDLL ()).c_str (), 
                   L"Vulkan-1.dll" ) ) {
    dll_log.Log (L"[  Vulkan  ] Additional Vulkan Initialization");
    dll_log.Log (L"[  Vulkan  ] ================================");

    dll_log.Log (L"[  Vulkan  ] Hooking Vk (1.x)");

    SK_CreateDLLHook2 (      L"vulkan-1.dll",
                              "vkCreateWin32SurfaceKHR",
                               vkCreateWin32SurfaceKHR_Detour,
      static_cast_p2p <void> (&SK::Vulkan::funcs.
                               vkCreateWin32SurfaceKHR)
                      );

    SK_CreateDLLHook2 (      L"vulkan-1.dll",
                              "vkQueuePresentKHR",
                               vkQueuePresentKHR_Detour,
      static_cast_p2p <void> (&SK::Vulkan::funcs.
                               vkQueuePresentKHR)
                      );

    SK_CreateDLLHook2 (      L"vulkan-1.dll",
                              "vkAcquireNextImageKHR",
                               vkAcquireNextImageKHR_Detour,
      static_cast_p2p <void> (&SK::Vulkan::funcs.
                               vkAcquireNextImageKHR)
                      );

    SK_CreateDLLHook2 (      L"vulkan-1.dll",
                              "vkCreateSwapchainKHR",
                               vkCreateSwapchainKHR_Detour,
      static_cast_p2p <void> (&SK::Vulkan::funcs.
                               vkCreateSwapchainKHR)
                      );

#if 0
    SK_CreateDLLHook2 (      L"vulkan-1.dll",
                              "vkDestroySwapchainKHR",
                               vkDestroySwapchainKHR_Detour,
      static_cast_p2p <void> (&SK::Vulkan::funcs.
                               vkDestroySwapchainKHR)
                      );
#endif

    MH_ApplyQueued ();
  }

  else {
    config.apis.Vulkan.hook = false;
    return;
  }

  InterlockedExchange (&__vk_ready, TRUE);
#endif
}

bool
SK::Vulkan::Startup (void)
{
  return SK_StartupCore (L"Vulkan", vulkan_init_callback);
}

bool
SK::Vulkan::Shutdown (void)
{
  if (hModVk != nullptr)
  {
    FreeLibrary (hModVk);
    hModVk = nullptr;
  }

  return SK_ShutdownCore (L"Vulkan");
}

BOOL SK_UsingVulkan (void)
{
  return ReadAcquire (&__vk_ready);
}

HMODULE                SK::Vulkan::hModVk = nullptr;
SK::Vulkan::vk_funcs_s SK::Vulkan::funcs { nullptr };
