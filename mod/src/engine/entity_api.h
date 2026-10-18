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
// cModelPart::SetAnim(char const*) on every part except part 0 when skip_first is set.
// Part 0 is the legs machine (the guide script). The torso/arms are later parts. Returns how many
// parts accepted the clip. Does not go through the entity script, so it does not create cBodyShotgun1npc.
int SetAnimOnParts(void* ent, const char* clip, bool skip_first);
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
// A fault is an engine exception thrown through the script VM (cVMachine::ExecuteDirect pushes its cState stack and
// only pops on a normal return). Every fault leaks VM state; a few hundred corrupt the VM (re/notes/yeti-bosses.md).
// Never retry a name that faulted.
std::string BodyStateName(void* ent);
int SetBodyState(void* ent, const char* name, bool reset);
// Model part count, and the body-state name of one part (part 0 is the legs). Empty if that part has no machine.
int ModelPartCount(void* ent);
std::string PartStateName(void* ent, int index);
// First upper-body part state that has left its spawn pose. Empty if every part is idle or missing.
std::string UpperBodyState(void* ent);
// cAIHuntBrain_Animal::FreezeBrain on the entity's brain (+0x200): brain Update returns early (no senses, path,
// body commands); animation and physics keep running.
bool FreezeBrain(void* ent, bool freeze);
// cObjCharacterEx::CharacterBrainFreeze(int). Fastcall: this in ecx, flag in edx.
// NPCs use cScrBrain_NPC, not cAIHuntBrain_Animal. The animal FreezeBrain is the wrong function.
bool CharacterBrainFreeze(void* ent, bool freeze);
// cObjCharacterEx::CharacterFreeze(int), same convention: the physics body stops simulating (no gravity).
// cObjCharacterEx::PlaySA pairs it with the no-collision flags so the two partners can overlap.
bool CharacterFreeze(void* ent, bool freeze);
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
// cObjScriptedEntity::SetVisible(int) (CommonDll7): +0xC0 bit 0 = hidden; showing relinks the BSP when linked.
bool SetVisible(void* ent, bool visible);
// cPhysicsObject::Freeze / Unfreeze (EngineDll7) on the entity's body (+0x118): the body flag +0xBC bit 2 stops
// the physics step integrating it. False when the entity has no body or the call faulted.
bool FreezeBody(void* ent, bool freeze);
// ---- sync attacks (re/notes/yeti-bosses.md) ----
// A sync attack ties the victim's model parts to the attacker's (cAIHuntTask_PlaySA::Begin CommonDll7 0xd19d0 and the
// attack task's TryPlaySA 0xcadf0: victim part 0 SetSyncWith(animal part 0), victim part 1 SetSyncWith(victim part 0)).
// cModelPart +0xf4 = first follower, +0x100 = the part this one follows, +0x10c = next follower; each a
// cModelPartLink {vt, owner entity, part index}. cModelPart::SetAnim(int) (EngineDll7 0x1e73b0) replays every clip on
// each follower and raises "Could not forward animation" when a follower cannot play it. Only the task's End undoes the
// links. Links between the entity's own parts, and with its attached children (weapons: parent id +0xA4 == ent id),
// are not "foreign".
int ForeignSyncCount(void* ent);   // links between this entity's parts and another character's; -1 on fault
int DetachForeignSync(void* ent);  // part vt +0x38 SetSyncWith(nullptr) on each; returns links removed, -1 on fault
// Name of the clip a model part plays now (cModelPart::GetCurrentAnim, cModelAnim +4). "" if none.
std::string PartClipName(void* ent, int part);
// True when any model part has a clip by that name (cModelPart::GetAnimIndex(char const*) >= 0).
bool HasClip(void* ent, const char* clip);
// cModelPart::SetAnim(char const*) on each part that has the clip. Returns parts that took it, -1 on fault.
int PlayClip(void* ent, const char* clip);
// Every part's body state machine to `name` (as cObjCharacterEx::PlaySA does with "CutScene"). Parts set, -1 on fault.
int SetPartsState(void* ent, const char* name, bool reset);
// cAIHuntBrainEx::IsTaskActive / StopTask(char const*) on the hunt brain (+0x200). StopTask calls the task's End
// (task vt +0x14): for AIHuntTask_PlaySA that unlinks the partner's parts and runs OnEndSA on both characters.
// StopTask removes the task at once: a cutscene script blocked in WaitForTask on it (it keeps the task pointer,
// CommonDll7 0x1f2c0) never resumes. BrainTask returns the task object (cAIHuntTaskEx*); its +4 is the state
// (1 running, 2 finished: the task's own finish sets 2, the waiting script resumes and then StopTasks it).
bool BrainTaskActive(void* ent, const char* taskClass);
bool StopBrainTask(void* ent, const char* taskClass);
void* BrainTask(void* ent, const char* taskClass);
int TaskState(void* task);  // -1 unreadable
// cObjCharacterEx::IsBrainFrozen: bit 4 of +0x204 (CharacterBrainFreeze). OnEndSA clears it.
bool IsCharacterBrainFrozen(void* ent);
// Manager vtable calls.
int AddEntity(void* mgr, void* ent);                             // +0x10, returns index or -1 on fault
bool RemoveEntity(void* mgr, void* ent);                         // +0x20 (ent, 1)

}  // namespace dh2::entity
