#pragma once
// Boss fights and scripted beats that play a special animation (re/notes/yeti-bosses.md).
//   - NPC PlayAnim beats: AITaskNPC_PlayAnim clips without a partner (MAP12A Reggie / Wirake "hurtdown", MAP2B Hugh
//     "hurtsit" / "hurtdown" / "syncguidegetup", cutscene "cs03" / "cs04"). The host commits each clip (kNpcAnim);
//     clients play it on their frozen copy and the NPC mirror holds that pose until the host's NPC leaves it.
//   - Boss props: the MAP12B falling trees (cObjAIHuntMoveObject) fall through their own PlaySA / OnEndSA, which the
//     host's Big Grimm starts. The host commits both (kPropSA / kPropSAEnd); clients replay them.
//   - Yeti throws (MAP12C): the host's rock exists on the host only. Clients throw a harmless copy along the same path
//     (kBossThrow); a host rock aimed at a puppet that passes through it hurts that player like the engine's own hit.
// [Bosses] in dh2coop.ini turns each part off.
#include <cstdint>
#include <string>

namespace dh2::coop::bosses {

// Client, NPC mirror (animals.cpp): true while a host PlayAnim clip holds this NPC's pose (do not write the body
// state or a leg clip). `hostState` is the host NPC's body-state name of this frame's sample.
bool NpcAnimHold(uint16_t id, const std::string& hostState);

}  // namespace dh2::coop::bosses
