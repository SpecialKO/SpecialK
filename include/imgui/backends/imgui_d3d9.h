// ImGui Win32 + DirectX9 binding
// In this binding, ImTextureID is used to store a 'LPDIRECT3DTEXTURE9' texture identifier. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#include <d3d9.h>

#include "../imgui.h"

IMGUI_API bool        ImGui_ImplDX9_Init     (void* hwnd, IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* pparams);
IMGUI_API void        ImGui_ImplDX9_Shutdown ();
IMGUI_API void        ImGui_ImplDX9_NewFrame ();
 
// Use if // Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API void        ImGui_ImplDX9_InvalidateDeviceObjects (D3DPRESENT_PARAMETERS* pparams);
IMGUI_API bool        ImGui_ImplDX9_CreateDeviceObjects     ();
