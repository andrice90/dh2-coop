#pragma once
// Thin, SEH-guarded wrappers over engine entity calls (EngineDll7 exports + vtable slots).
// Evidence: re/notes/lead-puppets.md (vtable slots from ??_7cObjEntity/cObjScriptedEntity/cObjHumanEx/cObjManager,
// spawn recipe from CommonDll7 cObjScriptedEntity::SpawnEntity and GameDll7 MOD_SvCreatePlayerEntity).
// All calls must run on the game's main thread.
#include <windows.h>

#include <cstdint>
#include <string>

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
// cObjScriptedEntity::UnlinkWithPhysics(1) (CommonDll7 export): removes the physics body from the physics
// manager and destroys it (+0x118 = 0). Without it the physics step writes the body position back over SetPos
// and the body collides/pushes. cObjHumanEx::Update skips a null body.
bool UnlinkPhysics(void* ent);
// Attachment (EngineDll7 exports): tag index of a named bone/tag on the entity's model (-1 none, -2 fault);
// Attach child to parentId at tag (flag 1 as scripts do; returns 1 on success); Detach from the parent.
int TagIndex(void* ent, const char* name);
int Attach(void* child, int parentId, int tag, int flag);
bool Detach(void* ent);
// Binds child's model part 0 to the owner's bone `tag` (cScrEntity::LinkPart equivalent): the child then follows
// the owner's animated bone. Returns 1 linked, 0 not found, -1 fault.
int LinkPart(void* owner, void* child, const char* tag);
// Reverse of LinkPart: the hand part's SetSyncWith(nullptr) (vt +0x38), which drops the mutual entity references of the
// sync ring. Call before removing either entity and before the world teardown.
// 1 unlinked, 2 nothing linked, 0 no model/bone/controller, -1 fault, -2 bad args.
int UnlinkPart(void* owner, const char* tag);
// Body state machine of the entity's model part 0 (scripted bodies: animals, items, NPCs). Name of the current
// state ("" if none/fault); switch to a named state (reset = call the machine's vt +0x60 first, as
// cObjItemEx::Activate does). 1 ok, 0 no machine, -1 fault.
std::string BodyStateName(void* ent);
int SetBodyState(void* ent, const char* name, bool reset);
// cAIHuntBrain_Animal::FreezeBrain on the entity's brain (+0x200): brain Update returns early (no senses, path,
// body commands); animation and physics keep running.
bool FreezeBrain(void* ent, bool freeze);
// cAIHuntAnimalInterface::FuncInflictDamage (CommonDll7, static __cdecl): subtracts `dmg` from health +0x19C.
bool InflictDamage(void* animal, float dmg);
// cScrAnimal vfunc +0x154 (GameDll FUN_1001e8b0): death presentation. Early-outs once +0x600 is set.
// Also AddDeadAnimal. Call once when a remote animal dies. Do not call it on the simulating host.
bool PresentDeath(void* animal);
// Animal attack-target list on the cAIHuntAnimalInterface singleton (CommonDll7 0x1aea7c). Animals pick the
// closest entry (cAIHuntBrain_Animal::GetClosestAttackTarget). Remove an entity before it is destroyed.
bool AddAttackTarget(void* ent);
bool RemoveAttackTarget(void* ent);
int AttackTargetCount();  // -1 when the interface is not available
// Manager vtable calls.
int AddEntity(void* mgr, void* ent);                             // +0x10, returns index or -1 on fault
bool RemoveEntity(void* mgr, void* ent);                         // +0x20 (ent, 1)

}  // namespace dh2::entity
