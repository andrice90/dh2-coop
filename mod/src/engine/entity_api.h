#pragma once
// Thin, SEH-guarded wrappers over engine entity calls (EngineDll7 exports + vtable slots).
// Evidence: re/notes/lead-puppets.md (vtable slots from ??_7cObjEntity/cObjScriptedEntity/cObjHumanEx/cObjManager,
// spawn recipe from CommonDll7 cObjScriptedEntity::SpawnEntity and GameDll7 MOD_SvCreatePlayerEntity).
// All calls must run on the game's main thread.
#include <windows.h>

#include <cstdint>

namespace dh2::entity {

struct Vec3 {
    float x = 0.f, y = 0.f, z = 0.f;
};

bool Bind(HMODULE engine);

// cObjManager::CreateEntity(const char* className, int) — static __cdecl. nullptr on failure.
void* Create(const char* className);
// Entity vtable calls. Return false if the call faulted.
bool HandleKey(void* ent, const char* key, const char* value);  // +0x14
bool Precache(void* ent);                                        // +0x54
bool Spawn(void* ent);                                           // +0x58
bool SetPos(void* ent, const Vec3& pos);                         // +0xA8
bool SetAngle(void* ent, const Vec3& angleDeg);                  // +0xB0 (x pitch, y yaw, z roll; degrees)
bool SetAnim(void* ent, const char* anim);                       // +0x174 (scripted entities only)
// cBspTree::UpdateEntity(*g_bsp, ent): (re)links the entity into BSP leaves from its world AABB (+0xC4..+0xD8).
// Runtime-spawned entities are not linked until this runs (link index +0x124 stays -1) → never drawn.
bool BspUpdate(void* ent);
// Current value of EngineDll7's `useServer` static (selects server/client BSP entity lists; set by
// cBspTree::UseServer). -1 if unresolved.
int UseServerValue();
// Manager vtable calls.
int AddEntity(void* mgr, void* ent);                             // +0x10, returns index or -1 on fault
bool RemoveEntity(void* mgr, void* ent);                         // +0x20 (ent, 1)

}  // namespace dh2::entity
