#include <imgui/imgui.h>

IMGUI_API bool  ImGui_ImplGL3_Init     (void);
IMGUI_API void  ImGui_ImplGL3_Shutdown (void);
IMGUI_API void  ImGui_ImplGL3_NewFrame (void);

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API void  ImGui_ImplGL3_InvalidateDeviceObjects (void);
IMGUI_API bool  ImGui_ImplGL3_CreateDeviceObjects     (void);