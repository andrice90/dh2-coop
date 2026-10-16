// Minimal Direct3D 8 COM interfaces for the co-op overlay.
// Method order matches d3d8.dll. Device Reset is vtable slot 14, Present is 15.
// Not a copy of the Wine/mingw-w64 headers or the Microsoft SDK. See README.md.
#pragma once

#include "d3d8types.h"
#include "d3d8caps.h"

// The ASI is built with WIN32_LEAN_AND_MEAN, so windows.h does not pull in IUnknown.
#include <unknwn.h>

#ifndef D3D_OK
#define D3D_OK S_OK
#endif

struct IDirect3D8;
struct IDirect3DDevice8;
struct IDirect3DSurface8;
struct IDirect3DSwapChain8;
struct IDirect3DResource8;
struct IDirect3DBaseTexture8;
struct IDirect3DTexture8;
struct IDirect3DVolumeTexture8;
struct IDirect3DCubeTexture8;
struct IDirect3DVertexBuffer8;
struct IDirect3DIndexBuffer8;
struct D3DGAMMARAMP;
struct D3DLIGHT8;
struct D3DMATERIAL8;
struct D3DCLIPSTATUS8;
struct D3DRASTER_STATUS;
struct D3DRECT;
struct D3DRECTPATCH_INFO;
struct D3DTRIPATCH_INFO;

// Surface is not an IDirect3DResource8. GetDesc is slot 8.
struct IDirect3DSurface8 : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice8** ppDevice) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID refguid, const void* data, DWORD data_size, DWORD flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID refguid, void* pData, DWORD* pSizeOfData) = 0;
    virtual HRESULT STDMETHODCALLTYPE FreePrivateData(REFGUID refguid) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetContainer(REFIID riid, void** ppContainer) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDesc(D3DSURFACE_DESC* pDesc) = 0;
    virtual HRESULT STDMETHODCALLTYPE LockRect(D3DLOCKED_RECT* locked_rect, const RECT* rect, DWORD flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE UnlockRect() = 0;
};

struct IDirect3DResource8 : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice8** ppDevice) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID refguid, const void* data, DWORD data_size, DWORD flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID refguid, void* pData, DWORD* pSizeOfData) = 0;
    virtual HRESULT STDMETHODCALLTYPE FreePrivateData(REFGUID refguid) = 0;
    virtual DWORD STDMETHODCALLTYPE SetPriority(DWORD priority_new) = 0;
    virtual DWORD STDMETHODCALLTYPE GetPriority() = 0;
    virtual void STDMETHODCALLTYPE PreLoad() = 0;
    virtual DWORD STDMETHODCALLTYPE GetType() = 0;
};

struct IDirect3DVertexBuffer8 : IDirect3DResource8 {
    virtual HRESULT STDMETHODCALLTYPE Lock(UINT offset_to_lock, UINT size_to_lock, BYTE** ppbData, DWORD flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE Unlock() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDesc(void* pDesc) = 0;
};

struct IDirect3DIndexBuffer8 : IDirect3DResource8 {
    virtual HRESULT STDMETHODCALLTYPE Lock(UINT offset_to_lock, UINT size_to_lock, BYTE** ppbData, DWORD flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE Unlock() = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDesc(void* pDesc) = 0;
};

struct IDirect3DBaseTexture8 : IDirect3DResource8 {
    virtual DWORD STDMETHODCALLTYPE SetLOD(DWORD lod_new) = 0;
    virtual DWORD STDMETHODCALLTYPE GetLOD() = 0;
    virtual DWORD STDMETHODCALLTYPE GetLevelCount() = 0;
};

// LockRect is slot 16: IUnknown(3) + resource(8) + base texture(3) + GetLevelDesc + GetSurfaceLevel.
struct IDirect3DTexture8 : IDirect3DBaseTexture8 {
    virtual HRESULT STDMETHODCALLTYPE GetLevelDesc(UINT level, D3DSURFACE_DESC* pDesc) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetSurfaceLevel(UINT level, IDirect3DSurface8** ppSurface) = 0;
    virtual HRESULT STDMETHODCALLTYPE LockRect(UINT level, D3DLOCKED_RECT* locked_rect, const RECT* rect, DWORD flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE UnlockRect(UINT level) = 0;
    virtual HRESULT STDMETHODCALLTYPE AddDirtyRect(const RECT* dirty_rect) = 0;
};

// Slots after IUnknown: Reset = 14, Present = 15, BeginScene = 34.
struct IDirect3DDevice8 : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE TestCooperativeLevel() = 0;
    virtual UINT STDMETHODCALLTYPE GetAvailableTextureMem() = 0;
    virtual HRESULT STDMETHODCALLTYPE ResourceManagerDiscardBytes(DWORD bytes) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDirect3D(IDirect3D8** ppD3D) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDeviceCaps(D3DCAPS8* pCaps) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDisplayMode(D3DDISPLAYMODE* pMode) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS* pParameters) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetCursorProperties(UINT x_hot, UINT y_hot, IDirect3DSurface8* cursor) = 0;
    virtual void STDMETHODCALLTYPE SetCursorPosition(UINT x, UINT y, DWORD flags) = 0;
    virtual BOOL STDMETHODCALLTYPE ShowCursor(BOOL show) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS* pPresent, IDirect3DSwapChain8** ppSwapChain) = 0;
    virtual HRESULT STDMETHODCALLTYPE Reset(D3DPRESENT_PARAMETERS* pPresent) = 0;
    virtual HRESULT STDMETHODCALLTYPE Present(const RECT* src, const RECT* dst, HWND dst_window, const RGNDATA* dirty) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetBackBuffer(UINT back_buffer, D3DBACKBUFFER_TYPE type, IDirect3DSurface8** ppBackBuffer) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetRasterStatus(D3DRASTER_STATUS* pStatus) = 0;
    virtual void STDMETHODCALLTYPE SetGammaRamp(DWORD flags, const D3DGAMMARAMP* ramp) = 0;
    virtual void STDMETHODCALLTYPE GetGammaRamp(D3DGAMMARAMP* pRamp) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateTexture(UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool, IDirect3DTexture8** ppTexture) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateVolumeTexture(UINT width, UINT height, UINT depth, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool, IDirect3DVolumeTexture8** ppTexture) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateCubeTexture(UINT edge, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool, IDirect3DCubeTexture8** ppTexture) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateVertexBuffer(UINT length, DWORD usage, DWORD fvf, D3DPOOL pool, IDirect3DVertexBuffer8** ppBuffer) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateIndexBuffer(UINT length, DWORD usage, D3DFORMAT format, D3DPOOL pool, IDirect3DIndexBuffer8** ppBuffer) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateRenderTarget(UINT width, UINT height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multi, BOOL lockable, IDirect3DSurface8** ppSurface) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateDepthStencilSurface(UINT width, UINT height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multi, IDirect3DSurface8** ppSurface) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateImageSurface(UINT width, UINT height, D3DFORMAT format, IDirect3DSurface8** ppSurface) = 0;
    virtual HRESULT STDMETHODCALLTYPE CopyRects(IDirect3DSurface8* src, const RECT* src_rects, UINT rect_count, IDirect3DSurface8* dst, const POINT* dst_points) = 0;
    virtual HRESULT STDMETHODCALLTYPE UpdateTexture(IDirect3DBaseTexture8* src, IDirect3DBaseTexture8* dst) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetFrontBuffer(IDirect3DSurface8* dest) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetRenderTarget(IDirect3DSurface8* color, IDirect3DSurface8* depth) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetRenderTarget(IDirect3DSurface8** ppColor) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDepthStencilSurface(IDirect3DSurface8** ppDepth) = 0;
    virtual HRESULT STDMETHODCALLTYPE BeginScene() = 0;
    virtual HRESULT STDMETHODCALLTYPE EndScene() = 0;
    virtual HRESULT STDMETHODCALLTYPE Clear(DWORD rect_count, const D3DRECT* rects, DWORD flags, D3DCOLOR color, float z, DWORD stencil) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetTransform(D3DTRANSFORMSTATETYPE state, const D3DMATRIX* matrix) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetTransform(D3DTRANSFORMSTATETYPE state, D3DMATRIX* matrix) = 0;
    virtual HRESULT STDMETHODCALLTYPE MultiplyTransform(D3DTRANSFORMSTATETYPE state, const D3DMATRIX* matrix) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetViewport(const D3DVIEWPORT8* viewport) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetViewport(D3DVIEWPORT8* viewport) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetMaterial(const D3DMATERIAL8* material) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetMaterial(D3DMATERIAL8* material) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetLight(DWORD index, const D3DLIGHT8* light) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetLight(DWORD index, D3DLIGHT8* light) = 0;
    virtual HRESULT STDMETHODCALLTYPE LightEnable(DWORD index, BOOL enable) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetLightEnable(DWORD index, BOOL* enable) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetClipPlane(DWORD index, const float* plane) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetClipPlane(DWORD index, float* plane) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetRenderState(D3DRENDERSTATETYPE state, DWORD value) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetRenderState(D3DRENDERSTATETYPE state, DWORD* value) = 0;
    virtual HRESULT STDMETHODCALLTYPE BeginStateBlock() = 0;
    virtual HRESULT STDMETHODCALLTYPE EndStateBlock(DWORD* pToken) = 0;
    virtual HRESULT STDMETHODCALLTYPE ApplyStateBlock(DWORD token) = 0;
    virtual HRESULT STDMETHODCALLTYPE CaptureStateBlock(DWORD token) = 0;
    virtual HRESULT STDMETHODCALLTYPE DeleteStateBlock(DWORD token) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateStateBlock(D3DSTATEBLOCKTYPE type, DWORD* pToken) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetClipStatus(const D3DCLIPSTATUS8* status) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetClipStatus(D3DCLIPSTATUS8* status) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetTexture(DWORD stage, IDirect3DBaseTexture8** ppTexture) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetTexture(DWORD stage, IDirect3DBaseTexture8* pTexture) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD* value) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value) = 0;
    virtual HRESULT STDMETHODCALLTYPE ValidateDevice(DWORD* pNumPasses) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetInfo(DWORD info_id, void* info, DWORD info_size) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPaletteEntries(UINT palette, const PALETTEENTRY* entries) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPaletteEntries(UINT palette, PALETTEENTRY* entries) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetCurrentTexturePalette(UINT palette) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentTexturePalette(UINT* palette) = 0;
    virtual HRESULT STDMETHODCALLTYPE DrawPrimitive(D3DPRIMITIVETYPE type, UINT start_vertex, UINT primitive_count) = 0;
    virtual HRESULT STDMETHODCALLTYPE DrawIndexedPrimitive(D3DPRIMITIVETYPE type, UINT min_index, UINT num_vertices, UINT start_index, UINT primitive_count) = 0;
    virtual HRESULT STDMETHODCALLTYPE DrawPrimitiveUP(D3DPRIMITIVETYPE type, UINT primitive_count, const void* data, UINT stride) = 0;
    virtual HRESULT STDMETHODCALLTYPE DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE type, UINT min_vertex, UINT vertex_count, UINT primitive_count, const void* index_data, D3DFORMAT index_format, const void* data, UINT stride) = 0;
    virtual HRESULT STDMETHODCALLTYPE ProcessVertices(UINT src_start, UINT dest_index, UINT vertex_count, IDirect3DVertexBuffer8* dest, DWORD flags) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateVertexShader(const DWORD* declaration, const DWORD* byte_code, DWORD* shader, DWORD usage) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetVertexShader(DWORD handle) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetVertexShader(DWORD* pHandle) = 0;
    virtual HRESULT STDMETHODCALLTYPE DeleteVertexShader(DWORD handle) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetVertexShaderConstant(DWORD reg, const void* data, DWORD count) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetVertexShaderConstant(DWORD reg, void* data, DWORD count) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetVertexShaderDeclaration(DWORD handle, void* data, DWORD* size) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetVertexShaderFunction(DWORD handle, void* data, DWORD* size) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetStreamSource(UINT stream, IDirect3DVertexBuffer8* buffer, UINT stride) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetStreamSource(UINT stream, IDirect3DVertexBuffer8** ppBuffer, UINT* pStride) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetIndices(IDirect3DIndexBuffer8* index_data, UINT base_vertex_index) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetIndices(IDirect3DIndexBuffer8** ppIndexData, UINT* pBaseVertexIndex) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreatePixelShader(const DWORD* byte_code, DWORD* shader) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPixelShader(DWORD handle) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPixelShader(DWORD* pHandle) = 0;
    virtual HRESULT STDMETHODCALLTYPE DeletePixelShader(DWORD handle) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPixelShaderConstant(DWORD reg, const void* data, DWORD count) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPixelShaderConstant(DWORD reg, void* data, DWORD count) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPixelShaderFunction(DWORD handle, void* data, DWORD* size) = 0;
    virtual HRESULT STDMETHODCALLTYPE DrawRectPatch(UINT handle, const float* segment_count, const D3DRECTPATCH_INFO* info) = 0;
    virtual HRESULT STDMETHODCALLTYPE DrawTriPatch(UINT handle, const float* segment_count, const D3DTRIPATCH_INFO* info) = 0;
    virtual HRESULT STDMETHODCALLTYPE DeletePatch(UINT handle) = 0;
};
