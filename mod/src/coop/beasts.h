#pragma once
// Species and weapons a client uses count the same as the host's (re/notes/beasts.md). Game thread only.
//   - Tranquilizer darts: a client's dart on a mirrored animal (cScrAnimal::OnHit 0x1a680) becomes a kTranqHit
//     request; the host runs the tranq branch (sleep timer, kill credit, overdose fail). MAP6/MAP7 give only darts.
//   - Mission cobras: a client's bullet or dart on a mirrored cobra (cObjCobra::OnHit 0x80440) becomes an AnimalHit,
//     which the host turns into the cobra's own death timer.
//   - Clip coverage for mirrors: whether a body state or leg clip can play on this model without the engine raising
//     "Could not forward animation" (a part linked to another part forwards every clip to it).
#include <string>

namespace dh2::coop::beasts {

// Client mirror, before SetBodyState(state). False when part 0 of the animal's model is linked (sync ring) to a part
// that has none of the clips this cAnimalLegs state plays: the engine forwards each clip to linked parts and raises
// when one cannot take it (cModelPart::SetAnim, EngineDll7 0x1e73b0). True for unknown states and unlinked parts.
bool MirrorStateOk(void* ent, const std::string& state);

// Client NPC mirror: the leg clip to SetAnim for a body state. `preferred` (animals.cpp NpcLegClip) when the model has
// it and its linked parts can take it, else the first fallback that can (girl/fiancée/ranger packs: `idle` for
// `idlenormalneutral`, `walk/forward` for the side walks), else nullptr: leave the pose the body state started (a
// missing clip stops the part's animation, the bind pose the audits saw).
const char* NpcLegClipFor(void* ent, const char* preferred);

// Client mirror: true when animals.cpp must keep this copy frozen although the host's state is CutScene. A thawed copy
// runs its own brain: an attack ring with a local hunter, puppet or NPC that outlives the host's, and the stale ring
// later raises "Could not forward animation ..." (MAP8 water croc: fatal dialog; MAP7 tiger and Reginald, MAP4A lioness).
// Water crocodiles (cObjCrocodile) are never thawed. Any other animal is thawed only while this process's camera is on
// a cutscene rail (a scripted cutscene waits on the animal's task), [Animals] ThawNeedsCamera=1 (default).
bool NoCutSceneThaw(void* ent);

}  // namespace dh2::coop::beasts
