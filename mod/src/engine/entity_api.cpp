#include "engine/entity_api.h"

#include <cstring>

#include <spdlog/spdlog.h>

#include "core/exports.h"

namespace dh2::entity {
namespace {

using CreateEntityFn = void*(__cdecl*)(const char* className, int flags);
using HandleKeyFn = int(__thiscall*)(void* self, char* key, char* value);
using VoidFn = void(__thiscall*)(void* self);
using VecFn = void(__thiscall*)(void* self, const Vec3* v);
using StrFn = void(__thiscall*)(void* self, const char* s);
using AddFn = int(__thiscall*)(void* self, void* ent);
using RemoveFn = int(__thiscall*)(void* self, void* ent, int flag);

constexpr uintptr_t kVtHandleKey = 0x14;
constexpr uintptr_t kVtPrecache = 0x54;
constexpr uintptr_t kVtSpawn = 0x58;
constexpr uintptr_t kVtSetPos = 0xA8;
constexpr uintptr_t kVtSetAngle = 0xB0;
constexpr uintptr_t kVtSetAnim = 0x174;
constexpr uintptr_t kMgrAdd = 0x10;
constexpr uintptr_t kMgrRemove = 0x20;

CreateEntityFn g_create = nullptr;
using BspUpdateFn = void(__thiscall*)(void* bsp, void* ent);
BspUpdateFn g_bsp_update = nullptr;
void** g_bsp_slot = nullptr;  // &g_bsp (cBspTree*)
int* g_use_server = nullptr;  // EngineDll7 RVA 0x6963dc, decoded from cBspTree::UseServer (+7: A3 imm32)

bool guarded_bsp_update(void* bsp, void* ent) {
    __try {
        g_bsp_update(bsp, ent);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// SEH helpers: no C++ objects with destructors in these functions (required for __try).
void* slot(void* obj, uintptr_t off) {
    void** vt = *reinterpret_cast<void***>(obj);
    return vt[off / sizeof(void*)];
}

void* guarded_create(const char* cls) {
    __try {
        return g_create(cls, 0);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

int guarded_handle_key(void* ent, char* k, char* v) {
    __try {
        return reinterpret_cast<HandleKeyFn>(slot(ent, kVtHandleKey))(ent, k, v);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

bool guarded_void(void* ent, uintptr_t off) {
    __try {
        reinterpret_cast<VoidFn>(slot(ent, off))(ent);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool guarded_vec(void* ent, uintptr_t off, const Vec3* v) {
    __try {
        reinterpret_cast<VecFn>(slot(ent, off))(ent, v);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool guarded_str(void* ent, uintptr_t off, const char* s) {
    __try {
        reinterpret_cast<StrFn>(slot(ent, off))(ent, s);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

int guarded_add(void* mgr, void* ent) {
    __try {
        return reinterpret_cast<AddFn>(slot(mgr, kMgrAdd))(mgr, ent);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

using UnlinkFn = void(__thiscall*)(void* self, int destroy);
UnlinkFn g_unlink = nullptr;  // CommonDll7 cObjScriptedEntity::UnlinkWithPhysics (not overridden by cObjHumanEx)

bool guarded_unlink(void* ent) {
    __try {
        g_unlink(ent, 1);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool guarded_remove(void* mgr, void* ent) {
    __try {
        reinterpret_cast<RemoveFn>(slot(mgr, kMgrRemove))(mgr, ent, 1);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

}  // namespace

bool Bind(HMODULE engine) {
    g_create = core::find_export_as<CreateEntityFn>(engine, "?CreateEntity@cObjManager@@SAPAVcObjEntity@@PBDH@Z");
    g_bsp_update = core::find_export_as<BspUpdateFn>(engine, "?UpdateEntity@cBspTree@@QAEXPAVcObjEntity@@@Z");
    g_bsp_slot = core::find_export_as<void**>(engine, "?g_bsp@@3PAVcBspTree@@A");
    if (auto* fn = core::find_export_as<const unsigned char*>(engine, "?UseServer@cBspTree@@SAXH@Z")) {
        const uintptr_t expect = reinterpret_cast<uintptr_t>(engine) + 0x6963dc;
        uint32_t imm = 0;
        std::memcpy(&imm, fn + 8, 4);
        if (fn[7] == 0xA3 && imm == expect) g_use_server = reinterpret_cast<int*>(imm);
        else spdlog::warn("entity_api: UseServer layout mismatch (op={:#x} imm={:#x})", fn[7], imm);
    }
    spdlog::info("entity_api: CreateEntity(char const*,int) = {} BspUpdate={} g_bsp={}",
                 core::describe_address(reinterpret_cast<void*>(g_create)),
                 core::describe_address(reinterpret_cast<void*>(g_bsp_update)), core::describe_address(g_bsp_slot));
    return g_create != nullptr;
}

void* Create(const char* className) {
    if (!g_create || !className) return nullptr;
    void* e = guarded_create(className);
    if (!e) spdlog::warn("entity_api: CreateEntity('{}') returned null or faulted", className);
    return e;
}

bool HandleKey(void* ent, const char* key, const char* value) {
    if (!ent || !key || !value) return false;
    char k[128], v[512];
    strncpy_s(k, key, _TRUNCATE);
    strncpy_s(v, value, _TRUNCATE);
    const int r = guarded_handle_key(ent, k, v);
    if (r < 0) spdlog::warn("entity_api: HandleKey('{}') faulted", key);
    return r > 0;
}

bool Precache(void* ent) { return ent && guarded_void(ent, kVtPrecache); }
bool Spawn(void* ent) { return ent && guarded_void(ent, kVtSpawn); }
bool SetPos(void* ent, const Vec3& pos) { return ent && guarded_vec(ent, kVtSetPos, &pos); }
bool SetAngle(void* ent, const Vec3& a) { return ent && guarded_vec(ent, kVtSetAngle, &a); }
bool SetAnim(void* ent, const char* anim) { return ent && anim && guarded_str(ent, kVtSetAnim, anim); }
bool BspUpdate(void* ent) {
    if (!ent || !g_bsp_update || !g_bsp_slot) return false;
    void* bsp = *g_bsp_slot;
    return bsp && guarded_bsp_update(bsp, ent);
}

int UseServerValue() { return g_use_server ? *g_use_server : -1; }

bool UnlinkPhysics(void* ent) {
    if (!ent) return false;
    if (!g_unlink) {
        if (HMODULE common = GetModuleHandleA("CommonDll7.dll"))
            g_unlink = core::find_export_as<UnlinkFn>(common, "?UnlinkWithPhysics@cObjScriptedEntity@@UAEXH@Z");
        if (!g_unlink) return false;
    }
    return guarded_unlink(ent);
}

int AddEntity(void* mgr, void* ent) { return (mgr && ent) ? guarded_add(mgr, ent) : -1; }
bool RemoveEntity(void* mgr, void* ent) { return mgr && ent && guarded_remove(mgr, ent); }

}  // namespace dh2::entity
