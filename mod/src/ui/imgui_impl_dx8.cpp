// Dear ImGui renderer for Direct3D 8.
// Ported from imgui v1.93.0 backends/imgui_impl_dx9.cpp. Differences are listed
// in re/notes/p2b-overlay.md. The game device is IDirect3DDevice8; there is no D3D9 device.

#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "ui/imgui_impl_dx8.h"

#include <d3d8.h>

#include <cstdint>
#include <cstring>

#include <spdlog/spdlog.h>

struct ImGui_ImplDX8_Data {
    IDirect3DDevice8* pd3dDevice = nullptr;
    IDirect3DVertexBuffer8* pVB = nullptr;
    IDirect3DIndexBuffer8* pIB = nullptr;
    int VertexBufferSize = 5000;
    int IndexBufferSize = 10000;

    bool backup_armed = false;
    DWORD state_token = 0;
    bool has_token = false;
    D3DMATRIX world = {};
    D3DMATRIX view = {};
    D3DMATRIX proj = {};
    bool has_xform = false;
    D3DVIEWPORT8 viewport = {};
    bool has_viewport = false;
    DWORD vs = 0;
    DWORD ps = 0;
    bool has_vs = false;
    bool has_ps = false;
    IDirect3DVertexBuffer8* saved_stream = nullptr;
    UINT saved_stride = 0;
    bool has_stream = false;
    IDirect3DIndexBuffer8* saved_indices = nullptr;
    UINT saved_base = 0;
    bool has_indices = false;
    IDirect3DBaseTexture8* saved_texture = nullptr;
    bool has_texture = false;
};

struct CUSTOMVERTEX {
    float pos[3];
    D3DCOLOR col;
    float uv[2];
};

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)

#ifdef IMGUI_USE_BGRA_PACKED_COLOR
#define IMGUI_COL_TO_DX_ARGB(_COL) (_COL)
#else
#define IMGUI_COL_TO_DX_ARGB(_COL) (((_COL) & 0xFF00FF00) | (((_COL) & 0xFF0000) >> 16) | (((_COL) & 0xFF) << 16))
#endif

static_assert(sizeof(D3DPRESENT_PARAMETERS) == 52, "D3D8 present parameters are 52 bytes");
static_assert(sizeof(D3DVIEWPORT8) == 24, "D3DVIEWPORT8");
static_assert(sizeof(D3DMATRIX) == 64, "D3DMATRIX");
static_assert(sizeof(D3DSURFACE_DESC) == 32, "D3DSURFACE_DESC");

static ImGui_ImplDX8_Data* ImGui_ImplDX8_GetBackendData() {
    return ImGui::GetCurrentContext() ? static_cast<ImGui_ImplDX8_Data*>(ImGui::GetIO().BackendRendererUserData) : nullptr;
}

static void log_d3d_fail(const char* what, HRESULT hr) {
    static int budget = 12;
    if (budget <= 0) return;
    --budget;
    spdlog::error("overlay: {} hr={:#010x}", what, static_cast<std::uint32_t>(hr));
}

static IDirect3DTexture8* tex_from_id(ImTextureID id) {
    return reinterpret_cast<IDirect3DTexture8*>(static_cast<uintptr_t>(id));
}

static ImTextureID id_from_tex(IDirect3DTexture8* tex) {
    return static_cast<ImTextureID>(reinterpret_cast<uintptr_t>(tex));
}

static void set_ortho(IDirect3DDevice8* device, float L, float R, float T, float B) {
    D3DMATRIX mat = {};
    mat._11 = 2.0f / (R - L);
    mat._22 = 2.0f / (T - B);
    mat._33 = 0.5f;
    mat._41 = (L + R) / (L - R);
    mat._42 = (T + B) / (B - T);
    mat._43 = 0.5f;
    mat._44 = 1.0f;
    device->SetTransform(D3DTS_PROJECTION, &mat);
}

static void ImGui_ImplDX8_SetupRenderState(ImDrawData* draw_data) {
    ImGui_ImplDX8_Data* bd = ImGui_ImplDX8_GetBackendData();
    IDirect3DDevice8* device = bd->pd3dDevice;

    D3DVIEWPORT8 vp = {};
    vp.X = 0;
    vp.Y = 0;
    vp.Width = static_cast<DWORD>(draw_data->DisplaySize.x);
    vp.Height = static_cast<DWORD>(draw_data->DisplaySize.y);
    vp.MinZ = 0.0f;
    vp.MaxZ = 1.0f;
    if (vp.Width == 0 || vp.Height == 0) return;
    device->SetViewport(&vp);

    // Fixed function. Pixel shader off, then the FVF through SetVertexShader (DX8 has no SetFVF).
    device->SetPixelShader(0);
    device->SetVertexShader(D3DFVF_CUSTOMVERTEX);
    device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
    device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    device->SetRenderState(D3DRS_ZENABLE, FALSE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
    device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    device->SetRenderState(D3DRS_FOGENABLE, FALSE);
    device->SetRenderState(D3DRS_RANGEFOGENABLE, FALSE);
    device->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
    device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    device->SetRenderState(D3DRS_CLIPPING, TRUE);
    device->SetRenderState(D3DRS_LIGHTING, FALSE);
    device->SetRenderState(D3DRS_COLORVERTEX, TRUE);
    device->SetRenderState(D3DRS_COLORWRITEENABLE,
                           D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE |
                               D3DCOLORWRITEENABLE_ALPHA);
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    device->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
    device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, 0);
    // DX8 filters live on the texture stage. There is no SetSamplerState.
    device->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
    device->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
    device->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTEXF_NONE);
    device->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
    device->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
    device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    device->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

    D3DMATRIX identity = {};
    identity._11 = identity._22 = identity._33 = identity._44 = 1.0f;
    device->SetTransform(D3DTS_WORLD, &identity);
    device->SetTransform(D3DTS_VIEW, &identity);
    const float L = draw_data->DisplayPos.x + 0.5f;
    const float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x + 0.5f;
    const float T = draw_data->DisplayPos.y + 0.5f;
    const float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y + 0.5f;
    set_ortho(device, L, R, T, B);
}

static void ImGui_ImplDX8_DrawCallback_ResetRenderState(const ImDrawList*, const ImDrawCmd*) {}

static void ImGui_ImplDX8_DrawCallback_SetSamplerLinear(const ImDrawList*, const ImDrawCmd*) {
    ImGui_ImplDX8_Data* bd = ImGui_ImplDX8_GetBackendData();
    bd->pd3dDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
    bd->pd3dDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
}

static void ImGui_ImplDX8_DrawCallback_SetSamplerNearest(const ImDrawList*, const ImDrawCmd*) {
    ImGui_ImplDX8_Data* bd = ImGui_ImplDX8_GetBackendData();
    bd->pd3dDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_POINT);
    bd->pd3dDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_POINT);
}

static void release_backup_refs(ImGui_ImplDX8_Data* bd) {
    if (bd->has_stream) {
        if (bd->saved_stream) bd->saved_stream->Release();
        bd->saved_stream = nullptr;
        bd->has_stream = false;
    }
    if (bd->has_indices) {
        if (bd->saved_indices) bd->saved_indices->Release();
        bd->saved_indices = nullptr;
        bd->has_indices = false;
    }
    if (bd->has_texture) {
        if (bd->saved_texture) bd->saved_texture->Release();
        bd->saved_texture = nullptr;
        bd->has_texture = false;
    }
    if (bd->has_token && bd->pd3dDevice) {
        bd->pd3dDevice->DeleteStateBlock(bd->state_token);
    }
    bd->has_token = false;
    bd->state_token = 0;
    bd->has_xform = false;
    bd->has_viewport = false;
    bd->has_vs = false;
    bd->has_ps = false;
    bd->backup_armed = false;
}

static void copy_texture_region(const ImU32* src, int src_pitch, ImU32* dst, int dst_pitch, int w, int h, bool convert) {
    for (int y = 0; y < h; ++y) {
        const auto* src_p = reinterpret_cast<const ImU32*>(reinterpret_cast<const unsigned char*>(src) + src_pitch * y);
        auto* dst_p = reinterpret_cast<ImU32*>(reinterpret_cast<unsigned char*>(dst) + dst_pitch * y);
        if (convert) {
            for (int x = w; x > 0; --x, ++src_p, ++dst_p) *dst_p = IMGUI_COL_TO_DX_ARGB(*src_p);
        } else {
            std::memcpy(dst_p, src_p, static_cast<size_t>(w) * 4u);
        }
    }
}

void ImGui_ImplDX8_UpdateTexture(ImTextureData* tex) {
    ImGui_ImplDX8_Data* bd = ImGui_ImplDX8_GetBackendData();
    if (!bd || !bd->pd3dDevice || !tex) return;

#ifdef IMGUI_USE_BGRA_PACKED_COLOR
    const bool convert = false;
#else
    const bool convert = tex->UseColors;
#endif

    if (tex->Status == ImTextureStatus_WantCreate) {
        IM_ASSERT(tex->TexID == ImTextureID_Invalid && tex->BackendUserData == nullptr);
        IM_ASSERT(tex->Format == ImTextureFormat_RGBA32);
        IDirect3DTexture8* dx_tex = nullptr;
        // Managed textures lock on DX8 without D3DUSAGE_DYNAMIC. Reset still drops our copy:
        // InvalidateDeviceObjects releases it and the atlas asks to be created again.
        HRESULT hr = bd->pd3dDevice->CreateTexture(static_cast<UINT>(tex->Width), static_cast<UINT>(tex->Height), 1, 0,
                                                   D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &dx_tex);
        const char* pool = "managed";
        if (FAILED(hr)) {
            hr = bd->pd3dDevice->CreateTexture(static_cast<UINT>(tex->Width), static_cast<UINT>(tex->Height), 1,
                                               D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &dx_tex);
            pool = "default";
        }
        if (FAILED(hr) || !dx_tex) {
            log_d3d_fail("CreateTexture", hr);
            return;
        }
        D3DLOCKED_RECT locked = {};
        if (dx_tex->LockRect(0, &locked, nullptr, 0) != D3D_OK) {
            log_d3d_fail("texture LockRect", E_FAIL);
            dx_tex->Release();
            return;
        }
        copy_texture_region(static_cast<const ImU32*>(tex->GetPixels()), tex->Width * 4, static_cast<ImU32*>(locked.pBits),
                            static_cast<int>(locked.Pitch), tex->Width, tex->Height, convert);
        dx_tex->UnlockRect(0);
        tex->SetTexID(id_from_tex(dx_tex));
        tex->SetStatus(ImTextureStatus_OK);
        spdlog::info("overlay: font texture created {}x{} fmt=A8R8G8B8 pool={}", tex->Width, tex->Height, pool);
    } else if (tex->Status == ImTextureStatus_WantUpdates) {
        IDirect3DTexture8* backend_tex = tex_from_id(tex->TexID);
        if (!backend_tex) return;
        RECT update_rect = {static_cast<LONG>(tex->UpdateRect.x), static_cast<LONG>(tex->UpdateRect.y),
                            static_cast<LONG>(tex->UpdateRect.x + tex->UpdateRect.w),
                            static_cast<LONG>(tex->UpdateRect.y + tex->UpdateRect.h)};
        D3DLOCKED_RECT locked = {};
        if (backend_tex->LockRect(0, &locked, &update_rect, 0) == D3D_OK) {
            for (ImTextureRect& r : tex->Updates) {
                auto* dst = static_cast<ImU32*>(locked.pBits) + (r.x - update_rect.left) +
                            (r.y - update_rect.top) * (locked.Pitch / 4);
                copy_texture_region(static_cast<const ImU32*>(tex->GetPixelsAt(r.x, r.y)), tex->Width * 4, dst,
                                    static_cast<int>(locked.Pitch), static_cast<int>(r.w), static_cast<int>(r.h), convert);
            }
            backend_tex->UnlockRect(0);
        }
        tex->SetStatus(ImTextureStatus_OK);
    } else if (tex->Status == ImTextureStatus_WantDestroy) {
        if (tex->TexID != ImTextureID_Invalid) {
            if (IDirect3DTexture8* backend_tex = tex_from_id(tex->TexID)) backend_tex->Release();
            tex->SetTexID(ImTextureID_Invalid);
        }
        tex->SetStatus(ImTextureStatus_Destroyed);
    }
}

static void update_textures(ImDrawData* draw_data) {
    if (!draw_data || !draw_data->Textures) return;
    for (ImTextureData* tex : *draw_data->Textures)
        if (tex->Status != ImTextureStatus_OK) ImGui_ImplDX8_UpdateTexture(tex);
}

bool ImGui_ImplDX8_PrepareDraw(ImDrawData* draw_data) {
    if (!draw_data) return false;
    update_textures(draw_data);
    if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f) return false;
    if (draw_data->TotalVtxCount <= 0 || draw_data->TotalIdxCount <= 0) return false;

    ImGui_ImplDX8_Data* bd = ImGui_ImplDX8_GetBackendData();
    if (!bd || !bd->pd3dDevice) return false;
    if (bd->backup_armed) ImGui_ImplDX8_FinishDraw();
    IDirect3DDevice8* device = bd->pd3dDevice;

    if (device->TestCooperativeLevel() != D3D_OK) return false;

    if (!bd->pVB || bd->VertexBufferSize < draw_data->TotalVtxCount) {
        if (bd->pVB) {
            bd->pVB->Release();
            bd->pVB = nullptr;
        }
        bd->VertexBufferSize = draw_data->TotalVtxCount + 5000;
        const HRESULT hr = device->CreateVertexBuffer(static_cast<UINT>(bd->VertexBufferSize) * sizeof(CUSTOMVERTEX),
                                                      D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFVF_CUSTOMVERTEX,
                                                      D3DPOOL_DEFAULT, &bd->pVB);
        if (FAILED(hr) || !bd->pVB) {
            log_d3d_fail("CreateVertexBuffer", hr);
            return false;
        }
    }
    if (!bd->pIB || bd->IndexBufferSize < draw_data->TotalIdxCount) {
        if (bd->pIB) {
            bd->pIB->Release();
            bd->pIB = nullptr;
        }
        bd->IndexBufferSize = draw_data->TotalIdxCount + 10000;
        const D3DFORMAT index_fmt = sizeof(ImDrawIdx) == 2 ? D3DFMT_INDEX16 : D3DFMT_INDEX32;
        const HRESULT hr = device->CreateIndexBuffer(static_cast<UINT>(bd->IndexBufferSize) * sizeof(ImDrawIdx),
                                                     D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, index_fmt, D3DPOOL_DEFAULT,
                                                     &bd->pIB);
        if (FAILED(hr) || !bd->pIB) {
            log_d3d_fail("CreateIndexBuffer", hr);
            return false;
        }
    }

    BYTE* vtx_bytes = nullptr;
    BYTE* idx_bytes = nullptr;
    if (bd->pVB->Lock(0, static_cast<UINT>(draw_data->TotalVtxCount) * sizeof(CUSTOMVERTEX), &vtx_bytes, D3DLOCK_DISCARD) < 0) {
        log_d3d_fail("vertex Lock", E_FAIL);
        return false;
    }
    if (bd->pIB->Lock(0, static_cast<UINT>(draw_data->TotalIdxCount) * sizeof(ImDrawIdx), &idx_bytes, D3DLOCK_DISCARD) < 0) {
        bd->pVB->Unlock();
        log_d3d_fail("index Lock", E_FAIL);
        return false;
    }
    auto* vtx_dst = reinterpret_cast<CUSTOMVERTEX*>(vtx_bytes);
    auto* idx_dst = reinterpret_cast<ImDrawIdx*>(idx_bytes);
    for (const ImDrawList* draw_list : draw_data->CmdLists) {
        const ImDrawVert* vtx_src = draw_list->VtxBuffer.Data;
        for (int i = 0; i < draw_list->VtxBuffer.Size; ++i) {
            vtx_dst->pos[0] = vtx_src->pos.x;
            vtx_dst->pos[1] = vtx_src->pos.y;
            vtx_dst->pos[2] = 0.0f;
            vtx_dst->col = IMGUI_COL_TO_DX_ARGB(vtx_src->col);
            vtx_dst->uv[0] = vtx_src->uv.x;
            vtx_dst->uv[1] = vtx_src->uv.y;
            ++vtx_dst;
            ++vtx_src;
        }
        std::memcpy(idx_dst, draw_list->IdxBuffer.Data, static_cast<size_t>(draw_list->IdxBuffer.Size) * sizeof(ImDrawIdx));
        idx_dst += draw_list->IdxBuffer.Size;
    }
    bd->pVB->Unlock();
    bd->pIB->Unlock();

    // Snapshot before CreateStateBlock so a failed block still releases the AddRefs.
    bd->has_xform = SUCCEEDED(device->GetTransform(D3DTS_WORLD, &bd->world)) &&
                    SUCCEEDED(device->GetTransform(D3DTS_VIEW, &bd->view)) &&
                    SUCCEEDED(device->GetTransform(D3DTS_PROJECTION, &bd->proj));
    bd->has_viewport = SUCCEEDED(device->GetViewport(&bd->viewport));
    bd->has_vs = SUCCEEDED(device->GetVertexShader(&bd->vs));
    bd->has_ps = SUCCEEDED(device->GetPixelShader(&bd->ps));
    bd->has_stream = SUCCEEDED(device->GetStreamSource(0, &bd->saved_stream, &bd->saved_stride));
    bd->has_indices = SUCCEEDED(device->GetIndices(&bd->saved_indices, &bd->saved_base));
    bd->has_texture = SUCCEEDED(device->GetTexture(0, &bd->saved_texture));

    const HRESULT sb = device->CreateStateBlock(D3DSBT_ALL, &bd->state_token);
    if (FAILED(sb)) {
        log_d3d_fail("CreateStateBlock(D3DSBT_ALL)", sb);
        release_backup_refs(bd);
        return false;
    }
    bd->has_token = true;
    bd->backup_armed = true;
    static bool logged_block = false;
    if (!logged_block) {
        logged_block = true;
        spdlog::info("overlay: CreateStateBlock(D3DSBT_ALL) hr={:#010x}", static_cast<std::uint32_t>(sb));
    }
    return true;
}

void ImGui_ImplDX8_RenderPrepared(ImDrawData* draw_data) {
    ImGui_ImplDX8_Data* bd = ImGui_ImplDX8_GetBackendData();
    if (!bd || !bd->pd3dDevice || !bd->backup_armed || !draw_data) return;
    IDirect3DDevice8* device = bd->pd3dDevice;

    device->SetStreamSource(0, bd->pVB, sizeof(CUSTOMVERTEX));
    device->SetVertexShader(D3DFVF_CUSTOMVERTEX);
    ImGui_ImplDX8_SetupRenderState(draw_data);

    int global_vtx_offset = 0;
    int global_idx_offset = 0;
    const ImVec2 clip_off = draw_data->DisplayPos;
    const int max_x = static_cast<int>(draw_data->DisplaySize.x);
    const int max_y = static_cast<int>(draw_data->DisplaySize.y);
    for (const ImDrawList* draw_list : draw_data->CmdLists) {
        for (int cmd_i = 0; cmd_i < draw_list->CmdBuffer.Size; ++cmd_i) {
            const ImDrawCmd* pcmd = &draw_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != nullptr) {
                if (pcmd->UserCallback == ImGui_ImplDX8_DrawCallback_ResetRenderState)
                    ImGui_ImplDX8_SetupRenderState(draw_data);
                else
                    pcmd->UserCallback(draw_list, pcmd);
                continue;
            }
            // DX8 has no scissor rect. Clip by fitting the viewport to the clip rect and
            // rebuilding the ortho matrix so vertex positions stay put. D3DRS_CLIPPING cuts the rest.
            ImVec2 clip_min(pcmd->ClipRect.x - clip_off.x, pcmd->ClipRect.y - clip_off.y);
            ImVec2 clip_max(pcmd->ClipRect.z - clip_off.x, pcmd->ClipRect.w - clip_off.y);
            if (clip_min.x < 0.0f) clip_min.x = 0.0f;
            if (clip_min.y < 0.0f) clip_min.y = 0.0f;
            if (clip_max.x > static_cast<float>(max_x)) clip_max.x = static_cast<float>(max_x);
            if (clip_max.y > static_cast<float>(max_y)) clip_max.y = static_cast<float>(max_y);
            const int x0 = static_cast<int>(clip_min.x);
            const int y0 = static_cast<int>(clip_min.y);
            const int x1 = static_cast<int>(clip_max.x);
            const int y1 = static_cast<int>(clip_max.y);
            if (x1 <= x0 || y1 <= y0) continue;

            D3DVIEWPORT8 vp = {};
            vp.X = static_cast<DWORD>(x0);
            vp.Y = static_cast<DWORD>(y0);
            vp.Width = static_cast<DWORD>(x1 - x0);
            vp.Height = static_cast<DWORD>(y1 - y0);
            vp.MinZ = 0.0f;
            vp.MaxZ = 1.0f;
            device->SetViewport(&vp);
            set_ortho(device, static_cast<float>(x0) + 0.5f, static_cast<float>(x1) + 0.5f, static_cast<float>(y0) + 0.5f,
                      static_cast<float>(y1) + 0.5f);

            const auto base = static_cast<UINT>(pcmd->VtxOffset + global_vtx_offset);
            device->SetIndices(bd->pIB, base);
            device->SetTexture(0, tex_from_id(pcmd->GetTexID()));
            device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, static_cast<UINT>(draw_list->VtxBuffer.Size),
                                         static_cast<UINT>(pcmd->IdxOffset + global_idx_offset),
                                         static_cast<UINT>(pcmd->ElemCount / 3));
        }
        global_idx_offset += draw_list->IdxBuffer.Size;
        global_vtx_offset += draw_list->VtxBuffer.Size;
    }
}

void ImGui_ImplDX8_FinishDraw() {
    ImGui_ImplDX8_Data* bd = ImGui_ImplDX8_GetBackendData();
    if (!bd || !bd->backup_armed || !bd->pd3dDevice) return;
    IDirect3DDevice8* device = bd->pd3dDevice;
    if (bd->has_token) {
        device->ApplyStateBlock(bd->state_token);
        device->DeleteStateBlock(bd->state_token);
        bd->has_token = false;
        bd->state_token = 0;
    }
    // Apply first, then write the snapshot again. If the block missed one of these, the game still gets it back.
    if (bd->has_xform) {
        device->SetTransform(D3DTS_WORLD, &bd->world);
        device->SetTransform(D3DTS_VIEW, &bd->view);
        device->SetTransform(D3DTS_PROJECTION, &bd->proj);
        bd->has_xform = false;
    }
    if (bd->has_viewport) {
        device->SetViewport(&bd->viewport);
        bd->has_viewport = false;
    }
    if (bd->has_vs) {
        device->SetVertexShader(bd->vs);
        bd->has_vs = false;
    }
    if (bd->has_ps) {
        device->SetPixelShader(bd->ps);
        bd->has_ps = false;
    }
    if (bd->has_stream) {
        device->SetStreamSource(0, bd->saved_stream, bd->saved_stride);
        if (bd->saved_stream) bd->saved_stream->Release();
        bd->saved_stream = nullptr;
        bd->has_stream = false;
    }
    if (bd->has_indices) {
        device->SetIndices(bd->saved_indices, bd->saved_base);
        if (bd->saved_indices) bd->saved_indices->Release();
        bd->saved_indices = nullptr;
        bd->has_indices = false;
    }
    if (bd->has_texture) {
        device->SetTexture(0, bd->saved_texture);
        if (bd->saved_texture) bd->saved_texture->Release();
        bd->saved_texture = nullptr;
        bd->has_texture = false;
    }
    bd->backup_armed = false;
}

bool ImGui_ImplDX8_CreateDeviceObjects() {
    ImGui_ImplDX8_Data* bd = ImGui_ImplDX8_GetBackendData();
    return bd && bd->pd3dDevice;
}

void ImGui_ImplDX8_InvalidateDeviceObjects() {
    ImGui_ImplDX8_Data* bd = ImGui_ImplDX8_GetBackendData();
    if (!bd || !bd->pd3dDevice) return;
    if (bd->backup_armed) ImGui_ImplDX8_FinishDraw();
    for (ImTextureData* tex : ImGui::GetPlatformIO().Textures) {
        if (tex->RefCount == 1) {
            tex->SetStatus(ImTextureStatus_WantDestroy);
            ImGui_ImplDX8_UpdateTexture(tex);
        }
    }
    if (bd->pVB) {
        bd->pVB->Release();
        bd->pVB = nullptr;
    }
    if (bd->pIB) {
        bd->pIB->Release();
        bd->pIB = nullptr;
    }
}

void ImGui_ImplDX8_NewFrame() {
    ImGui_ImplDX8_Data* bd = ImGui_ImplDX8_GetBackendData();
    IM_ASSERT(bd != nullptr && "Context or backend not initialized! Did you call ImGui_ImplDX8_Init()?");
    IM_UNUSED(bd);
}

bool ImGui_ImplDX8_Init(IDirect3DDevice8* device) {
    ImGuiIO& io = ImGui::GetIO();
    IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");
    if (!device) return false;

    ImGui_ImplDX8_Data* bd = IM_NEW(ImGui_ImplDX8_Data)();
    io.BackendRendererUserData = static_cast<void*>(bd);
    io.BackendRendererName = "imgui_impl_dx8";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;

    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.Renderer_TextureMaxWidth = 4096;
    platform_io.Renderer_TextureMaxHeight = 4096;
    platform_io.DrawCallback_ResetRenderState = ImGui_ImplDX8_DrawCallback_ResetRenderState;
    platform_io.DrawCallback_SetSamplerLinear = ImGui_ImplDX8_DrawCallback_SetSamplerLinear;
    platform_io.DrawCallback_SetSamplerNearest = ImGui_ImplDX8_DrawCallback_SetSamplerNearest;

    bd->pd3dDevice = device;
    bd->pd3dDevice->AddRef();
    return true;
}

void ImGui_ImplDX8_Shutdown() {
    ImGui_ImplDX8_Data* bd = ImGui_ImplDX8_GetBackendData();
    IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

    ImGui_ImplDX8_InvalidateDeviceObjects();
    if (bd->pd3dDevice) bd->pd3dDevice->Release();

    io.BackendRendererName = nullptr;
    io.BackendRendererUserData = nullptr;
    io.BackendFlags &= ~(ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures);
    platform_io.ClearRendererHandlers();
    IM_DELETE(bd);
}

#endif
