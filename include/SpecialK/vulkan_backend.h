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

#ifndef __SK__VULKAN_BACKEND_H__
#define __SK__VULKAN_BACKEND_H__

#define VK_USE_PLATFORM_WIN32_KHR
#define VK_NO_PROTOTYPES

#include <vulkan/vulkan.h>

namespace SK
{
  namespace Vulkan
  {

    bool Startup  (void);
    bool Shutdown (void);

    extern HMODULE hModVk;

    struct vk_funcs_s {
      PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
      PFN_vkQueuePresentKHR       vkQueuePresentKHR;
      PFN_vkCreateSwapchainKHR    vkCreateSwapchainKHR;
      PFN_vkDestroySwapchainKHR   vkDestroySwapchainKHR;
      PFN_vkAcquireNextImageKHR   vkAcquireNextImageKHR;
    } extern funcs;
  }
}

BOOL SK_UsingVulkan (void);

#endif /* __SK__VULKAN_BACKEND_H__ */