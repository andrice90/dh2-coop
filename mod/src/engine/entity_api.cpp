#include "engine/entity_api.h"

#include <cstring>
#include <string>

#include <spdlog/spdlog.h>

#include "core/exports.h"
#include "core/safe_mem.h"

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
struct LinkDiag {
    void* ctrl = nullptr;
    void* ctrl_vt = nullptr;
    void* link_fn = nullptr;
    void* part = nullptr;
    void* part_model = nullptr;
    void* part_owner = nullptr;
    void* part_ring_before = nullptr;
    void* hand_sync_before = nullptr;
    void* hand_next_before = nullptr;
    void* hand_next_after = nullptr;
} g_link_diag;
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
        // Sync-ring diagnostics (cModelPartLink +4 = owner entity): who is already in the weapon part's ring?
        g_link_diag.part_ring_before = *reinterpret_cast<void**>(static_cast<char*>(part) + 0xf4 + 4);
        g_link_diag.hand_sync_before = *reinterpret_cast<void**>(static_cast<char*>(ctrl) + 0x100 + 4);
        g_link_diag.hand_next_before = *reinterpret_cast<void**>(static_cast<char*>(ctrl) + 0x10c + 4);
        reinterpret_cast<LinkFn>(slot(ctrl, 0x38))(ctrl, part);
        g_link_diag.hand_next_after = *reinterpret_cast<void**>(static_cast<char*>(ctrl) + 0x10c + 4);
        g_link_diag.ctrl = ctrl;
        g_link_diag.ctrl_vt = *reinterpret_cast<void**>(ctrl);
        g_link_diag.link_fn = slot(ctrl, 0x38);
        g_link_diag.part = part;
        g_link_diag.part_model = *reinterpret_cast<void**>(static_cast<char*>(part) + 0x68);
        g_link_diag.part_owner = g_link_diag.part_model
                                     ? *reinterpret_cast<void**>(static_cast<char*>(g_link_diag.part_model) + 0xf8)
                                     : nullptr;
        return 1;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

// Reverse of LinkPart. The "controller" at bone +0x138 is itself a model part, and vt +0x38 is
// cModelPart::SetSyncWith(part) (EngineDll7 0x1e7050; GameDll's cPartHumanTorso thunk). It builds a sync ring:
// hand +0x100 -> weapon part, weapon part +0xf4 -> hand. Each cModelPartLink InternalAddRefs the other part's
// owner entity, so puppet and weapon hold references on each other. The world teardown then destroys one of them
// twice (2026-10-16 20:42/20:51 crash in cSndDrvDS::WaveRelease from StopServer -> DeleteAllObjects, only with
// puppet weapons). SetSyncWith(nullptr) is the engine's own way out: it unthreads the ring and Unlinks both links.
//
// First-chance AV on every leave / weapon switch on story maps (MAP3B, 2026-10-17 17:09; re/notes/puppet-polish.md):
// the tail of SetSyncWith(nullptr) runs the part's state machine Begin (part +0xd8, vt +0x20). On those maps the
// hand part holds a cScriptedBodyStateMachine (EngineDll7 vtable 0x428750), whose Begin goes through the script VM
// (cVMachine::FunctionCall) and calls a stale function pointer (the class list entry for the missing
// cBodyShotgun1npc / cBodyHandgun3npc body script). Free-hunt maps hold a plain cBodyStateMachine there and are fine.
// The state machine is hidden from the call (part +0xd8 = 0 while it runs, put back after) when it is a scripted
// one, so the unlink still unthreads the ring and releases both references, minus the Begin that faults.
struct UnlinkDiag {
    void* ctrl = nullptr;
    void* sm = nullptr;
    void* sm_vt = nullptr;
    void* sm_vt20 = nullptr;
    bool hid_sm = false;
    void* synced = nullptr;  // the weapon entity the hand was synced with
    // Sync ring links (cModelPartLink, +4 = owner entity): before and after SetSyncWith(nullptr), all must end null.
    void* w_back_before = nullptr;  // weapon part +0xf4
    void* w_back_after = nullptr;
    void* hand_next_before = nullptr;  // hand part +0x10c
    void* hand_next_after = nullptr;
    void* hand_sync_after = nullptr;  // hand part +0x100
} g_unlink_diag;
void* g_scripted_sm_vt = nullptr;  // EngineDll7 ??_7cScriptedBodyStateMachine@@6B@

void restore_sm_slot(void** sm_slot, void* sm) {
    __try {
        *sm_slot = sm;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

int guarded_unlink_part(void* owner, const char* tag) {
    void** sm_slot = nullptr;
    void* sm = nullptr;
    int result = -1;
    __try {
        void* owner_model = *reinterpret_cast<void**>(static_cast<char*>(owner) + 0xE4);
        if (!owner_model) return 0;
        void* bone = reinterpret_cast<GetTagFn>(slot(owner_model, 0xe4))(owner_model, tag);
        if (!bone) return 0;
        void* ctrl = *reinterpret_cast<void**>(static_cast<char*>(bone) + 0x138);
        if (!ctrl) return 0;
        void* synced = *reinterpret_cast<void**>(static_cast<char*>(ctrl) + 0x100 + 4);  // link +4 = owner entity
        if (!synced) return 2;
        g_unlink_diag.ctrl = ctrl;
        g_unlink_diag.synced = synced;
        sm_slot =reinterpret_cast<void**>(static_cast<char*>(ctrl) + 0xd8);
        sm = *sm_slot;
        g_unlink_diag.sm = sm;
        g_unlink_diag.sm_vt = sm ? *reinterpret_cast<void**>(sm) : nullptr;
        g_unlink_diag.sm_vt20 = g_unlink_diag.sm_vt ? slot(sm, 0x20) : nullptr;
        if (sm && g_scripted_sm_vt && g_unlink_diag.sm_vt == g_scripted_sm_vt) {
            *sm_slot = nullptr;
            g_unlink_diag.hid_sm = true;
        }
        void* wpart = nullptr;
        if (void* wmodel = *reinterpret_cast<void**>(static_cast<char*>(synced) + 0xE4))
            wpart = reinterpret_cast<GetPartFn>(slot(wmodel, 0xf0))(wmodel, 0);
        if (wpart) g_unlink_diag.w_back_before = *reinterpret_cast<void**>(static_cast<char*>(wpart) + 0xf4 + 4);
        g_unlink_diag.hand_next_before = *reinterpret_cast<void**>(static_cast<char*>(ctrl) + 0x10c + 4);
        reinterpret_cast<LinkFn>(slot(ctrl, 0x38))(ctrl, nullptr);
        if (wpart) g_unlink_diag.w_back_after = *reinterpret_cast<void**>(static_cast<char*>(wpart) + 0xf4 + 4);
        g_unlink_diag.hand_next_after = *reinterpret_cast<void**>(static_cast<char*>(ctrl) + 0x10c + 4);
        g_unlink_diag.hand_sync_after = *reinterpret_cast<void**>(static_cast<char*>(ctrl) + 0x100 + 4);
        result = 1;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        result = -1;
    }
    if (g_unlink_diag.hid_sm && sm_slot) restore_sm_slot(sm_slot, sm);
    return result;
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

using PartSetAnimFn = int(__thiscall*)(void* part, const char* name);
PartSetAnimFn g_part_set_anim = nullptr;

int guarded_set_anim_parts(void* ent, const char* clip, bool skip_first) {
    __try {
        if (!g_part_set_anim) {
            if (HMODULE engine = GetModuleHandleA("EngineDll7.dll"))
                g_part_set_anim = reinterpret_cast<PartSetAnimFn>(
                    GetProcAddress(engine, "?SetAnim@cModelPart@@UAEHPBD@Z"));
        }
        if (!g_part_set_anim || !clip) return 0;
        void* model = *reinterpret_cast<void**>(static_cast<char*>(ent) + 0xE4);
        if (!model) return 0;
        const short parts = reinterpret_cast<short(__thiscall*)(void*)>(slot(model, 0xf8))(model);
        int applied = 0;
        for (int i = skip_first ? 1 : 0; i < parts; ++i) {
            void* part = reinterpret_cast<GetPartFn>(slot(model, 0xf0))(model, i);
            if (!part) continue;
            if (g_part_set_anim(part, clip) > 0) ++applied;
        }
        return applied;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
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

using AttackListFn = void(__thiscall*)(void* iface, void* ent);
using AttackCountFn = int(__thiscall*)(void* iface);
AttackListFn g_add_attack = nullptr;
AttackListFn g_remove_attack = nullptr;
AttackCountFn g_attack_count = nullptr;
void** g_animal_iface = nullptr;  // CommonDll7 DAT 0x1aea7c, read by GetClosestAttackTarget

bool bind_attack_list() {
    if (g_add_attack && g_remove_attack && g_attack_count && g_animal_iface) return true;
    HMODULE common = GetModuleHandleA("CommonDll7.dll");
    if (!common) return false;
    g_add_attack = core::find_export_as<AttackListFn>(common, "?AddAttackTarget@cAIHuntAnimalInterface@@QAEXPAVcObjEntity@@@Z");
    g_remove_attack =
        core::find_export_as<AttackListFn>(common, "?RemoveAttackTarget@cAIHuntAnimalInterface@@QAEXPAVcObjEntity@@@Z");
    g_attack_count = core::find_export_as<AttackCountFn>(common, "?GetAttackTargetCount@cAIHuntAnimalInterface@@QAEHXZ");
    g_animal_iface = reinterpret_cast<void**>(reinterpret_cast<char*>(common) + 0x1aea7c);
    return g_add_attack && g_remove_attack && g_attack_count;
}

bool guarded_attack_list(AttackListFn fn, void* iface, void* ent) {
    __try {
        fn(iface, ent);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

int guarded_attack_count(void* iface) {
    __try {
        return g_attack_count(iface);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
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

const char* guarded_part_state(void* ent, int index, int* count) {
    __try {
        void* model = *reinterpret_cast<void**>(static_cast<char*>(ent) + 0xE4);
        if (!model) return nullptr;
        const short parts = reinterpret_cast<short(__thiscall*)(void*)>(slot(model, 0xf8))(model);
        if (count) *count = parts;
        if (index < 0 || index >= parts) return nullptr;
        void* part = reinterpret_cast<GetPartFn>(slot(model, 0xf0))(model, index);
        if (!part) return nullptr;
        void* sm = *reinterpret_cast<void**>(static_cast<char*>(part) + 0xd8);
        return sm ? reinterpret_cast<const char*(__thiscall*)(void*)>(slot(sm, 0x80))(sm) : nullptr;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

// ---- sync rings (entity_api.h "sync attacks"; re/notes/yeti-bosses.md) ----
constexpr uintptr_t kPartFollowers = 0xf4;  // cModelPartLink: first part that follows this one
constexpr uintptr_t kPartMaster = 0x100;    // cModelPartLink: the part this one follows
constexpr uintptr_t kPartNext = 0x10c;      // cModelPartLink: next follower of the same master
constexpr uintptr_t kVtSyncWith = 0x38;     // cModelPart::SetSyncWith(cModelPart*) (EngineDll7 0x1e7050)
constexpr uintptr_t kVtPartSetAnimName = 0x58;  // cModelPart::SetAnim(char const*), as cObjScriptedEntity::SetAnim calls it
constexpr int kMaxLinks = 16;

using PartCountFn = unsigned short(__thiscall*)(void* model);
using AnimIndexFn = int(__thiscall*)(void* part, const char* name);
using GetAnimFn = void*(__thiscall*)(void* part, int index);
using TaskActiveFn = void*(__thiscall*)(void* brain, const char* name);
AnimIndexFn g_anim_index = nullptr;  // cModelPart::GetAnimIndex(char const*) const
GetAnimFn g_get_anim = nullptr;      // cModelPart::GetAnim(int)
TaskActiveFn g_task_active = nullptr;  // cAIHuntBrainEx::IsTaskActive(char const*)

void bind_part_fns() {
    if (g_anim_index && g_get_anim) return;
    if (HMODULE engine = GetModuleHandleA("EngineDll7.dll")) {
        g_anim_index = reinterpret_cast<AnimIndexFn>(GetProcAddress(engine, "?GetAnimIndex@cModelPart@@QBEHPBD@Z"));
        g_get_anim = reinterpret_cast<GetAnimFn>(GetProcAddress(engine, "?GetAnim@cModelPart@@QAEPAVcModelAnim@@H@Z"));
    }
}

int part_count(void* model) { return reinterpret_cast<PartCountFn>(slot(model, 0xf8))(model); }

// cModelPartLink::operator-> (EngineDll7 0x1e51c0): the linked part, or null when the link is not valid.
char* link_target(char* link) {
    void* owner = *reinterpret_cast<void**>(link + 4);
    const int index = *reinterpret_cast<int*>(link + 8);
    if (!owner || index < 0) return nullptr;
    void* model = *reinterpret_cast<void**>(static_cast<char*>(owner) + 0xE4);
    if (!model || index >= part_count(model)) return nullptr;
    return static_cast<char*>(reinterpret_cast<GetPartFn>(slot(model, 0xf0))(model, index));
}

// Another character, not one of this entity's attached children (a weapon's part is the master of the hand bone).
bool foreign_owner(void* ent, void* owner) {
    if (!owner || owner == ent) return false;
    const int16_t id = *reinterpret_cast<int16_t*>(static_cast<char*>(ent) + 0x04);
    const int16_t parent = *reinterpret_cast<int16_t*>(static_cast<char*>(owner) + 0xA4);
    return parent != id;
}

// Parts whose SetSyncWith(nullptr) removes a foreign link: this entity's parts that follow a foreign part, and
// foreign parts that follow one of this entity's parts.
int guarded_foreign_links(void* ent, char** out, int cap) {
    __try {
        void* model = *reinterpret_cast<void**>(static_cast<char*>(ent) + 0xE4);
        if (!model) return 0;
        const int parts = part_count(model);
        int n = 0;
        for (int i = 0; i < parts && i < kMaxLinks; ++i) {
            char* part = static_cast<char*>(reinterpret_cast<GetPartFn>(slot(model, 0xf0))(model, i));
            if (!part) continue;
            char* master_link = part + kPartMaster;
            if (foreign_owner(ent, *reinterpret_cast<void**>(master_link + 4)) && link_target(master_link) && n < cap)
                out[n++] = part;
            char* link = part + kPartFollowers;
            for (int guard = 0; guard < kMaxLinks; ++guard) {
                char* follower = link_target(link);
                if (!follower) break;
                if (foreign_owner(ent, *reinterpret_cast<void**>(link + 4)) && n < cap) out[n++] = follower;
                link = follower + kPartNext;
            }
        }
        return n;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

bool guarded_sync_with_null(char* part) {
    __try {
        reinterpret_cast<void(__thiscall*)(void*, void*)>(slot(part, kVtSyncWith))(part, nullptr);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

const char* guarded_part_clip(void* ent, int index) {
    __try {
        void* model = *reinterpret_cast<void**>(static_cast<char*>(ent) + 0xE4);
        if (!model || !g_get_anim || index < 0 || index >= part_count(model)) return nullptr;
        char* part = static_cast<char*>(reinterpret_cast<GetPartFn>(slot(model, 0xf0))(model, index));
        if (!part) return nullptr;
        const short cur = *reinterpret_cast<short*>(part + 0x90);  // cModelPart::GetAnimIndex() (0x16ea50)
        if (cur < 0) return nullptr;
        char* anim = static_cast<char*>(g_get_anim(part, cur));
        return anim ? *reinterpret_cast<const char**>(anim + 4) : nullptr;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

// play=false: count the parts that have the clip. play=true: also SetAnim it on those parts.
int guarded_clip_parts(void* ent, const char* clip, bool play) {
    __try {
        void* model = *reinterpret_cast<void**>(static_cast<char*>(ent) + 0xE4);
        if (!model || !g_anim_index) return 0;
        const int parts = part_count(model);
        int n = 0;
        for (int i = 0; i < parts && i < kMaxLinks; ++i) {
            void* part = reinterpret_cast<GetPartFn>(slot(model, 0xf0))(model, i);
            if (!part || g_anim_index(part, clip) < 0) continue;
            if (play) reinterpret_cast<int(__thiscall*)(void*, const char*)>(slot(part, kVtPartSetAnimName))(part, clip);
            ++n;
        }
        return n;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

int guarded_parts_state(void* ent, const char* name, bool reset) {
    __try {
        void* model = *reinterpret_cast<void**>(static_cast<char*>(ent) + 0xE4);
        if (!model) return 0;
        const int parts = part_count(model);
        int n = 0;
        for (int i = 0; i < parts && i < kMaxLinks; ++i) {
            char* part = static_cast<char*>(reinterpret_cast<GetPartFn>(slot(model, 0xf0))(model, i));
            void* sm = part ? *reinterpret_cast<void**>(part + 0xd8) : nullptr;
            if (!sm) continue;
            if (reset) reinterpret_cast<void(__thiscall*)(void*)>(slot(sm, 0x60))(sm);
            reinterpret_cast<void(__thiscall*)(void*, const char*)>(slot(sm, 0x7c))(sm, name);
            ++n;
        }
        return n;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

void* guarded_task_active(void* brain, const char* name) {
    __try {
        return g_task_active(brain, name);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

bool guarded_stop_task(void* brain, const char* name) {
    __try {
        // cScrCharacterEx::StopTask (CommonDll7 0x1edd0) calls the brain's vt +0x34 with the class name.
        reinterpret_cast<void(__thiscall*)(void*, const char*)>(slot(brain, 0x34))(brain, name);
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
    g_scripted_sm_vt = core::find_export(engine, "??_7cScriptedBodyStateMachine@@6B@");
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

int SetAnimOnParts(void* ent, const char* clip, bool skip_first) {
    return ent && clip ? guarded_set_anim_parts(ent, clip, skip_first) : 0;
}
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

std::string PartStateName(void* ent, int index) {
    if (!ent) return {};
    const char* s = guarded_part_state(ent, index, nullptr);
    if (!s) return {};
    char buf[64] = {};
    for (size_t i = 0; i + 1 < sizeof buf && s[i]; ++i) buf[i] = s[i];
    return buf;
}

int ModelPartCount(void* ent) {
    int count = 0;
    if (ent) guarded_part_state(ent, 0, &count);
    return count;
}

std::string UpperBodyState(void* ent) {
    const int n = ModelPartCount(ent);
    std::string fallback;
    for (int i = 1; i < n; ++i) {
        const std::string s = PartStateName(ent, i);
        if (s.empty() || s == "NoChange") continue;
        if (fallback.empty()) fallback = s;
        if (s != "Idle" && s != "Activate" && s != "Changein") return s;
    }
    return fallback;
}
int SetBodyState(void* ent, const char* name, bool reset) { return (ent && name) ? guarded_set_state(ent, name, reset) : 0; }

bool FreezeBrain(void* ent, bool freeze) {
    if (!ent) return false;
    if (!g_freeze_brain) {
        if (HMODULE common = GetModuleHandleA("CommonDll7.dll"))
            g_freeze_brain = core::find_export_as<FreezeFn>(common, "?FreezeBrain@cAIHuntBrain_Animal@@QAEXH@Z");
        if (!g_freeze_brain) return false;
    }
    void* brain = nullptr;  // cObjCharacterEx brain
    if (!core::safe_read(static_cast<char*>(ent) + 0x200, brain) || !brain) return false;
    // Only a cAIHuntBrain_Animal: FreezeBrain writes its byte +0x79, and an NPC's +0x200 is a cAIBrain_NPC (review
    // M1/M2: the NPC death and reset paths called this on Hugh). The animal brains are CommonDll7 classes derived from
    // it (wolf/lion/... vtables +0x131118, +0x1311f0, +0x1312c8 on MAP2A); cAIBrain_NPC is GameDll7's (vtable
    // +0x112d30). Refuse any vtable outside CommonDll7.
    void* vt = nullptr;
    HMODULE common = GetModuleHandleA("CommonDll7.dll");
    HMODULE vt_mod = nullptr;
    const bool read = core::safe_read(brain, vt) && vt;
    if (read)
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           static_cast<LPCSTR>(vt), &vt_mod);
    if (!read || !common || vt_mod != common) {
        static void* last_refused = nullptr;
        if (vt != last_refused) {
            last_refused = vt;
            spdlog::warn("entity_api: FreezeBrain refused: brain vt {} is not a CommonDll7 animal brain",
                         core::describe_address(vt));
        }
        return false;
    }
    return guarded_freeze(brain, freeze ? 1 : 0);
}

bool IsHuntBrainFrozen(void* ent) {
    char* brain = nullptr;
    uint8_t flag = 0;
    return ent && core::safe_read(static_cast<char*>(ent) + 0x200, brain) && brain &&
           core::safe_read(brain + 0x79, flag) && flag != 0;
}

using CharFreezeFn = void(__fastcall*)(void* self, int freeze);
CharFreezeFn g_char_freeze = nullptr;

bool guarded_char_freeze(void* ent, int freeze) {
    __try {
        g_char_freeze(ent, freeze);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool CharacterBrainFreeze(void* ent, bool freeze) {
    if (!ent) return false;
    if (!g_char_freeze) {
        if (HMODULE common = GetModuleHandleA("CommonDll7.dll"))
            g_char_freeze = reinterpret_cast<CharFreezeFn>(
                GetProcAddress(common, "?CharacterBrainFreeze@cObjCharacterEx@@QAIXH@Z"));
        if (!g_char_freeze) return false;
    }
    return guarded_char_freeze(ent, freeze ? 1 : 0);
}

CharFreezeFn g_body_freeze = nullptr;  // cObjCharacterEx::CharacterFreeze: the physics body stops simulating

bool guarded_body_freeze(void* ent, int freeze) {
    __try {
        g_body_freeze(ent, freeze);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool CharacterFreeze(void* ent, bool freeze) {
    if (!ent) return false;
    if (!g_body_freeze) {
        if (HMODULE common = GetModuleHandleA("CommonDll7.dll"))
            g_body_freeze = reinterpret_cast<CharFreezeFn>(
                GetProcAddress(common, "?CharacterFreeze@cObjCharacterEx@@QAIXH@Z"));
        if (!g_body_freeze) return false;
    }
    return guarded_body_freeze(ent, freeze ? 1 : 0);
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

namespace {
using SetVisibleFn = void(__thiscall*)(void* self, int visible);
using PhysToggleFn = void(__thiscall*)(void* phys);
SetVisibleFn g_set_visible = nullptr;
PhysToggleFn g_phys_freeze = nullptr;
PhysToggleFn g_phys_unfreeze = nullptr;

bool guarded_set_visible(void* ent, int visible) {
    __try {
        g_set_visible(ent, visible);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool guarded_phys_toggle(PhysToggleFn fn, void* phys) {
    __try {
        fn(phys);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}
}  // namespace

bool SetVisible(void* ent, bool visible) {
    if (!ent) return false;
    if (!g_set_visible) {
        if (HMODULE common = GetModuleHandleA("CommonDll7.dll"))
            g_set_visible = core::find_export_as<SetVisibleFn>(common, "?SetVisible@cObjScriptedEntity@@UAEXH@Z");
        if (!g_set_visible) return false;
    }
    return guarded_set_visible(ent, visible ? 1 : 0);
}

bool FreezeBody(void* ent, bool freeze) {
    if (!ent) return false;
    if (!g_phys_freeze || !g_phys_unfreeze) {
        if (HMODULE engine = GetModuleHandleA("EngineDll7.dll")) {
            g_phys_freeze = core::find_export_as<PhysToggleFn>(engine, "?Freeze@cPhysicsObject@@UAEXXZ");
            g_phys_unfreeze = core::find_export_as<PhysToggleFn>(engine, "?Unfreeze@cPhysicsObject@@UAEXXZ");
        }
        if (!g_phys_freeze || !g_phys_unfreeze) return false;
    }
    void* phys = nullptr;
    if (!core::safe_read(static_cast<char*>(ent) + 0x118, phys) || !phys) return false;
    return guarded_phys_toggle(freeze ? g_phys_freeze : g_phys_unfreeze, phys);
}

bool AddAttackTarget(void* ent) {
    if (!ent || !bind_attack_list()) return false;
    void* iface = *g_animal_iface;
    return iface && guarded_attack_list(g_add_attack, iface, ent);
}

bool RemoveAttackTarget(void* ent) {
    if (!ent || !bind_attack_list()) return false;
    void* iface = *g_animal_iface;
    return iface && guarded_attack_list(g_remove_attack, iface, ent);
}

int AttackTargetCount() {
    if (!bind_attack_list()) return -1;
    void* iface = *g_animal_iface;
    return iface ? guarded_attack_count(iface) : -1;
}

int TagIndex(void* ent, const char* name) { return (ent && name && g_tag_index) ? guarded_tag_index(ent, name) : -2; }
int Attach(void* child, int parentId, int tag, int flag) {
    return (child && g_attach) ? guarded_attach(child, parentId, tag, flag) : -1;
}
bool Detach(void* ent) { return ent && g_detach && guarded_detach(ent); }
int UnlinkPart(void* owner, const char* tag) {
    if (!owner || !tag) return -2;
    g_unlink_diag = {};
    int owner_before = -99, owner_after = -99, w_before = -99, w_after = -99;
    core::safe_read(static_cast<char*>(owner) + 0xb8, owner_before);
    const int r = guarded_unlink_part(owner, tag);
    core::safe_read(static_cast<char*>(owner) + 0xb8, owner_after);
    if (g_unlink_diag.synced) {
        core::safe_read(static_cast<char*>(g_unlink_diag.synced) + 0xb8, w_after);
    }
    (void)w_before;
    spdlog::info("entity_api: UnlinkPart owner={:#x} tag='{}' -> {} (SetSyncWith(nullptr)) ctrl={:#x} sm={:#x} "
                 "sm_vt={} vt+0x20={} hid_sm={} refs owner {}->{} weapon after {} ring weapon+0xf4 {:#x}->{:#x} "
                 "hand+0x10c {:#x}->{:#x} hand+0x100 after {:#x}",
                 reinterpret_cast<uintptr_t>(owner), tag, r, reinterpret_cast<uintptr_t>(g_unlink_diag.ctrl),
                 reinterpret_cast<uintptr_t>(g_unlink_diag.sm), core::describe_address(g_unlink_diag.sm_vt),
                 core::describe_address(g_unlink_diag.sm_vt20), g_unlink_diag.hid_sm, owner_before, owner_after,
                 w_after, reinterpret_cast<uintptr_t>(g_unlink_diag.w_back_before),
                 reinterpret_cast<uintptr_t>(g_unlink_diag.w_back_after),
                 reinterpret_cast<uintptr_t>(g_unlink_diag.hand_next_before),
                 reinterpret_cast<uintptr_t>(g_unlink_diag.hand_next_after),
                 reinterpret_cast<uintptr_t>(g_unlink_diag.hand_sync_after));
    return r;
}

int LinkPart(void* owner, void* child, const char* tag) {
    if (!owner || !child || !tag) return -1;
    g_link_diag = {};
    const int r = guarded_link_part(owner, child, tag);
    spdlog::info(
        "entity_api: LinkPart owner={:#x} child={:#x} tag='{}' -> {} ctrl={:#x} vt={} fn={} part={:#x} model={:#x} "
        "model+0xf8={:#x}",
        reinterpret_cast<uintptr_t>(owner), reinterpret_cast<uintptr_t>(child), tag, r,
        reinterpret_cast<uintptr_t>(g_link_diag.ctrl), core::describe_address(g_link_diag.ctrl_vt),
        core::describe_address(g_link_diag.link_fn), reinterpret_cast<uintptr_t>(g_link_diag.part),
        reinterpret_cast<uintptr_t>(g_link_diag.part_model), reinterpret_cast<uintptr_t>(g_link_diag.part_owner));
    auto vt_of = [](void* e) -> std::string {
        void* vt = nullptr;
        return (e && core::safe_read(e, vt)) ? core::describe_address(vt) : std::string("-");
    };
    spdlog::info("entity_api: LinkPart ring: part+0xf4 before={:#x} ({}) hand+0x100 before={:#x} hand+0x10c before={:#x} "
                 "after={:#x} ({})",
                 reinterpret_cast<uintptr_t>(g_link_diag.part_ring_before), vt_of(g_link_diag.part_ring_before),
                 reinterpret_cast<uintptr_t>(g_link_diag.hand_sync_before),
                 reinterpret_cast<uintptr_t>(g_link_diag.hand_next_before),
                 reinterpret_cast<uintptr_t>(g_link_diag.hand_next_after), vt_of(g_link_diag.hand_next_after));
    return r;
}

int ForeignSyncCount(void* ent) {
    if (!ent) return 0;
    char* parts[kMaxLinks] = {};
    return guarded_foreign_links(ent, parts, kMaxLinks);
}

int DetachForeignSync(void* ent) {
    if (!ent) return 0;
    char* parts[kMaxLinks] = {};
    const int n = guarded_foreign_links(ent, parts, kMaxLinks);
    if (n <= 0) return n;
    int removed = 0;
    for (int i = 0; i < n; ++i) {
        if (guarded_sync_with_null(parts[i])) ++removed;
        else return -1;
    }
    return removed;
}

std::string PartClipName(void* ent, int part) {
    if (!ent) return {};
    bind_part_fns();
    const char* s = guarded_part_clip(ent, part);
    return s ? core::safe_cstr(s, 64) : std::string();
}

bool HasClip(void* ent, const char* clip) {
    if (!ent || !clip || !clip[0]) return false;
    bind_part_fns();
    return guarded_clip_parts(ent, clip, false) > 0;
}

int PlayClip(void* ent, const char* clip) {
    if (!ent || !clip || !clip[0]) return 0;
    bind_part_fns();
    return guarded_clip_parts(ent, clip, true);
}

int SetPartsState(void* ent, const char* name, bool reset) {
    return (ent && name) ? guarded_parts_state(ent, name, reset) : 0;
}

void* BrainTask(void* ent, const char* taskClass) {
    if (!ent || !taskClass) return nullptr;
    if (!g_task_active) {
        if (HMODULE common = GetModuleHandleA("CommonDll7.dll"))
            g_task_active = reinterpret_cast<TaskActiveFn>(
                GetProcAddress(common, "?IsTaskActive@cAIHuntBrainEx@@UAEPAVcAIHuntTaskEx@@PBD@Z"));
        if (!g_task_active) return nullptr;
    }
    void* brain = nullptr;
    if (!core::safe_read(static_cast<char*>(ent) + 0x200, brain) || !brain) return nullptr;
    return guarded_task_active(brain, taskClass);
}

bool BrainTaskActive(void* ent, const char* taskClass) { return BrainTask(ent, taskClass) != nullptr; }

int TaskState(void* task) {
    int state = -1;
    if (!task || !core::safe_read(static_cast<char*>(task) + 4, state)) return -1;
    return state;
}

bool StopBrainTask(void* ent, const char* taskClass) {
    if (!BrainTaskActive(ent, taskClass)) return false;
    void* brain = nullptr;
    core::safe_read(static_cast<char*>(ent) + 0x200, brain);
    return brain && guarded_stop_task(brain, taskClass);
}

bool IsCharacterBrainFrozen(void* ent) {
    uint32_t flags = 0;
    return ent && core::safe_read(static_cast<char*>(ent) + 0x204, flags) && (flags & 0x10) != 0;
}

int AddEntity(void* mgr, void* ent) { return (mgr && ent) ? guarded_add(mgr, ent) : -1; }
bool RemoveEntity(void* mgr, void* ent) { return mgr && ent && guarded_remove(mgr, ent); }

}  // namespace dh2::entity
