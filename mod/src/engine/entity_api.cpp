#include "engine/entity_api.h"

#include <cstring>
#include <string>

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
constexpr uintptr_t kVtPresentDeath = 0x154;  // cScrAnimal: FUN_1001e8b0
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

// Attachment (EngineDll7 cObjEntity, lead decompile): Attach(parentId, tagIndex, flag, tVect3 offset) stores
// parent id +0xA4, tag +0xA6, flags +0xA8 and registers the child in the parent's slots +0x168; SetPos/UpdatePos
// then follow GetTagPos(parent, tag). GetTagIndex(name) = model vt +0xe8.
using TagIndexFn = int(__thiscall*)(void* self, const char* name);
using AttachFn = int(__thiscall*)(void* self, int parentId, int tag, int flag, Vec3 offset);
using DetachFn = void(__thiscall*)(void* self);
TagIndexFn g_tag_index = nullptr;
AttachFn g_attach = nullptr;
DetachFn g_detach = nullptr;

int guarded_tag_index(void* ent, const char* name) {
    __try {
        return g_tag_index(ent, name);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -2;
    }
}

int guarded_attach(void* ent, int parent, int tag, int flag) {
    __try {
        return g_attach(ent, parent, tag, flag, Vec3{});
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

bool guarded_detach(void* ent) {
    __try {
        g_detach(ent);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// cScrEntity::LinkPart (CommonDll7 0x3f590) without the script wrapper: owner model GetTag(name) (vt +0xe4)
// -> bone; bone +0x138 = controller; controller vt +0x38 (child model part 0 = child model vt +0xf0(0)).
using GetTagFn = void*(__thiscall*)(void* model, const char* name);
using GetPartFn = void*(__thiscall*)(void* model, int index);
using LinkFn = void(__thiscall*)(void* ctrl, void* part);
int guarded_link_part(void* owner, void* child, const char* tag) {
    __try {
        void* owner_model = *reinterpret_cast<void**>(static_cast<char*>(owner) + 0xE4);
        void* child_model = *reinterpret_cast<void**>(static_cast<char*>(child) + 0xE4);
        if (!owner_model || !child_model) return 0;
        void* bone = reinterpret_cast<GetTagFn>(slot(owner_model, 0xe4))(owner_model, tag);
        if (!bone) return 0;
        void* ctrl = *reinterpret_cast<void**>(static_cast<char*>(bone) + 0x138);
        if (!ctrl) return 0;
        void* part = reinterpret_cast<GetPartFn>(slot(child_model, 0xf0))(child_model, 0);
        if (!part) return 0;
        reinterpret_cast<LinkFn>(slot(ctrl, 0x38))(ctrl, part);
        return 1;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

// Body state machine of model part 0 (pattern from cObjItemEx::GetCurrentState/Activate, CommonDll7): model vt
// +0xf8 part count (short), +0xf0(0) part 0; part +0xd8 = state machine; vt +0x80 current state name,
// vt +0x7c(name) switch state (Activate calls vt +0x60 first).
void* body_states(void* ent) {
    void* model = *reinterpret_cast<void**>(static_cast<char*>(ent) + 0xE4);
    if (!model) return nullptr;
    const short parts = reinterpret_cast<short(__thiscall*)(void*)>(slot(model, 0xf8))(model);
    if (parts == 0) return nullptr;
    void* part = reinterpret_cast<GetPartFn>(slot(model, 0xf0))(model, 0);
    return part ? *reinterpret_cast<void**>(static_cast<char*>(part) + 0xd8) : nullptr;
}

const char* guarded_state_name(void* ent) {
    __try {
        void* sm = body_states(ent);
        return sm ? reinterpret_cast<const char*(__thiscall*)(void*)>(slot(sm, 0x80))(sm) : nullptr;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

int guarded_set_state(void* ent, const char* name, bool reset) {
    __try {
        void* sm = body_states(ent);
        if (!sm) return 0;
        if (reset) reinterpret_cast<void(__thiscall*)(void*)>(slot(sm, 0x60))(sm);
        reinterpret_cast<void(__thiscall*)(void*, const char*)>(slot(sm, 0x7c))(sm, name);
        return 1;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

using FreezeFn = void(__thiscall*)(void* brain, int freeze);
FreezeFn g_freeze_brain = nullptr;  // CommonDll7 cAIHuntBrain_Animal::FreezeBrain (brain +0x79)
using InflictFn = void(__cdecl*)(void* animal, float dmg);
InflictFn g_inflict = nullptr;  // CommonDll7 cAIHuntAnimalInterface::FuncInflictDamage

bool guarded_freeze(void* brain, int freeze) {
    __try {
        g_freeze_brain(brain, freeze);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool guarded_inflict(void* animal, float dmg) {
    __try {
        g_inflict(animal, dmg);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool guarded_present_death(void* animal) {
    __try {
        reinterpret_cast<VoidFn>(slot(animal, kVtPresentDeath))(animal);
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
    g_tag_index = core::find_export_as<TagIndexFn>(engine, "?GetTagIndex@cObjEntity@@QAEHPBD@Z");
    g_attach = core::find_export_as<AttachFn>(engine, "?Attach@cObjEntity@@QAEHHHHUtVect3@@@Z");
    g_detach = core::find_export_as<DetachFn>(engine, "?Detach@cObjEntity@@QAEXXZ");
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

std::string BodyStateName(void* ent) {
    const char* s = ent ? guarded_state_name(ent) : nullptr;
    if (!s) return {};
    char buf[64] = {};
    for (size_t i = 0; i + 1 < sizeof buf && s[i]; ++i) buf[i] = s[i];
    return buf;
}
int SetBodyState(void* ent, const char* name, bool reset) { return (ent && name) ? guarded_set_state(ent, name, reset) : 0; }

bool FreezeBrain(void* ent, bool freeze) {
    if (!ent) return false;
    if (!g_freeze_brain) {
        if (HMODULE common = GetModuleHandleA("CommonDll7.dll"))
            g_freeze_brain = core::find_export_as<FreezeFn>(common, "?FreezeBrain@cAIHuntBrain_Animal@@QAEXH@Z");
        if (!g_freeze_brain) return false;
    }
    void* brain = *reinterpret_cast<void**>(static_cast<char*>(ent) + 0x200);  // cObjCharacterEx brain
    return brain && guarded_freeze(brain, freeze ? 1 : 0);
}

bool InflictDamage(void* animal, float dmg) {
    if (!animal) return false;
    if (!g_inflict) {
        if (HMODULE common = GetModuleHandleA("CommonDll7.dll"))
            g_inflict = core::find_export_as<InflictFn>(common,
                                                        "?FuncInflictDamage@cAIHuntAnimalInterface@@KAXPAXM@Z");
        if (!g_inflict) {
            spdlog::warn("entity_api: FuncInflictDamage unresolved");
            return false;
        }
    }
    const bool ok = guarded_inflict(animal, dmg);
    spdlog::info("entity_api: FuncInflictDamage ent={:#x} dmg={:.2f} ok={}", reinterpret_cast<uintptr_t>(animal), dmg,
                 ok);
    return ok;
}

bool PresentDeath(void* animal) {
    if (!animal) return false;
    const bool ok = guarded_present_death(animal);
    spdlog::info("entity_api: PresentDeath ent={:#x} ok={}", reinterpret_cast<uintptr_t>(animal), ok);
    return ok;
}

int TagIndex(void* ent, const char* name) { return (ent && name && g_tag_index) ? guarded_tag_index(ent, name) : -2; }
int Attach(void* child, int parentId, int tag, int flag) {
    return (child && g_attach) ? guarded_attach(child, parentId, tag, flag) : -1;
}
bool Detach(void* ent) { return ent && g_detach && guarded_detach(ent); }
int LinkPart(void* owner, void* child, const char* tag) {
    return (owner && child && tag) ? guarded_link_part(owner, child, tag) : -1;
}

int AddEntity(void* mgr, void* ent) { return (mgr && ent) ? guarded_add(mgr, ent) : -1; }
bool RemoveEntity(void* mgr, void* ent) { return mgr && ent && guarded_remove(mgr, ent); }

}  // namespace dh2::entity
