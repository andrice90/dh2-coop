// Dear ImGui renderer for Direct3D 8, ported from imgui v1.93.0 imgui_impl_dx9.
// There is no platform backend here: the overlay feeds display size and dt itself.
#pragma once
#include "imgui.h"
#ifndef IMGUI_DISABLE

struct IDirect3DDevice8;

IMGUI_IMPL_API bool ImGui_ImplDX8_Init(IDirect3DDevice8* device);
IMGUI_IMPL_API void ImGui_ImplDX8_Shutdown();
IMGUI_IMPL_API void ImGui_ImplDX8_NewFrame();
IMGUI_IMPL_API bool ImGui_ImplDX8_CreateDeviceObjects();
IMGUI_IMPL_API void ImGui_ImplDX8_InvalidateDeviceObjects();
IMGUI_IMPL_API void ImGui_ImplDX8_UpdateTexture(ImTextureData* tex);

// Upload geometry and capture device state. Does not call BeginScene.
// False means nothing was captured and the caller must not draw or finish.
IMGUI_IMPL_API bool ImGui_ImplDX8_PrepareDraw(ImDrawData* draw_data);
// Issue draws. The caller is inside BeginScene, after a true PrepareDraw.
IMGUI_IMPL_API void ImGui_ImplDX8_RenderPrepared(ImDrawData* draw_data);
// ApplyStateBlock / DeleteStateBlock and restore the extra saved state.
// Required after PrepareDraw returned true, including when BeginScene failed.
IMGUI_IMPL_API void ImGui_ImplDX8_FinishDraw();

#endif
