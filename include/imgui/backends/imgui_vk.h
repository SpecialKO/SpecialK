#include <vulkan/vulkan.h>
#include <imgui/imgui.h>

#define IMGUI_VK_QUEUED_FRAMES 2

struct ImGui_ImplVulkan_Init_Data
{
  VkAllocationCallbacks* allocator;
  VkPhysicalDevice       gpu;
  VkDevice               device;
  VkRenderPass           render_pass;
  VkPipelineCache        pipeline_cache;
  VkDescriptorPool       descriptor_pool;
  void (*check_vk_result)(VkResult err);
};

IMGUI_API bool    ImGui_ImplVulkan_Init     (ImGui_ImplVulkan_Init_Data *init_data);
IMGUI_API void    ImGui_ImplVulkan_Shutdown ();
IMGUI_API void    ImGui_ImplVulkan_NewFrame ();
IMGUI_API void    ImGui_ImplVulkan_Render   (VkCommandBuffer command_buffer);

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API void    ImGui_ImplVulkan_InvalidateFontUploadObjects ();
IMGUI_API void    ImGui_ImplVulkan_InvalidateDeviceObjects     ();
IMGUI_API bool    ImGui_ImplVulkan_CreateFontsTexture          (VkCommandBuffer command_buffer);
IMGUI_API bool    ImGui_ImplVulkan_CreateDeviceObjects         ();
