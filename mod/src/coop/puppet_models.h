#pragma once
// Bodies a player can pick in the lobby for how the others see them. The index travels in Hello / PlayerJoined
// (protocol v3); the build id check means every peer has this same table.
// The puppet clips come from game.ap (legs) and shotgun1.ap / handgun3.ap (arms), so a body needs the shared
// biped rig of the male NPCs. The teen girl, nephew and fiancee packs lack the idle clip and are not offered.
namespace dh2::coop {

struct PuppetModel {
    const char* label;  // shown in the lobby
    const char* model;  // "model_name" key: skin, then the animation packs
};

int PuppetModelCount();
// Out of range gives entry 0 (the guide).
const PuppetModel& PuppetModelAt(int index);

}  // namespace dh2::coop
