#pragma once
// Host: scripts and AI that ask for "the hunter" get the nearest living player (re/notes/hunter-nearest.md).
// [Story] HunterNearest=1 (default off). Game thread only.
#include <cstdint>

namespace dh2::coop::hunter_nearest {

// True when [Story] HunterNearest=1 (read at init; the hooks still act only while hosting a session).
bool Enabled();
// The target name a puppet carries ("coop_p<id>") so name lookups ("target" task params, GetDistBetween2Ais)
// can resolve it. Stable storage.
const char* PuppetName(uint8_t playerId);
// puppets.cpp after a remote puppet spawned: gives it PuppetName(id) (cObjBaseEntity::SetTargetName) when enabled.
void NamePuppet(void* ent, uint8_t playerId);
// The living player nearest to `pos`: the host's own hunter (alive) or a living puppet. Null when none is known.
// `dist_sq` gets the squared distance when not null.
void* NearestLivingPlayer(const float pos[3], float* dist_sq);
// Called by puppets.cpp right before a puppet entity is destroyed (player left, session end) and from the map
// unload: drops every engine reference this module and npc_follow made to it (stimuli, NPC commands).
void OnPuppetRemoving(void* ent);

}  // namespace dh2::coop::hunter_nearest

namespace dh2::coop::npc_follow {
// NPC `npc` was just given `target` (a puppet) by a name substitution or the per-frame player lookup.
void NoteTarget(void* npc, void* target);
// `ent` is about to be destroyed: NPCs whose last player target was `ent` drop their queued commands
// (cAIBrain_NPC::ClearAllInfoTask, the script StopAllTask) and re-pick next frame.
void Forget(void* ent);
}  // namespace dh2::coop::npc_follow
