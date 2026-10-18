#pragma once
// Player-count difficulty (spec re/notes/player-scale.md, result re/notes/scale.md). extra = connected players - 1.
// The host decides; clients copy. Solo, and a host with no client, play the retail fight.
//  - One body, more health: a MakeBoss animal, or a lone large animal starting a scripted attack, gets
//    max = base * (1 + 0.25 * extra). The host commits each max (events::kScaleMax).
//  - More bodies: every peer adds the same reserve animals (coop_<SPECIES>_<n>) at map load; the host keeps `extra`
//    of them in the fight while a script animal of that species attacks. Their deaths do not reach the mission.
// The functions below are what each hook does after (or around) the original call, so the lead can fold them into
// another module's hook on the same function.
#include <cstdint>

namespace dh2::coop::scale {

// GameDll 0x8d970 AddAnimal(species, flag, name): the map script created a story animal.
void AfterAddAnimal(int species, const char* name);
// GameDll 0x8cab0: the map script's AddAnimalOnMap returned. Creates the reserve pool (same order on every peer).
void BeforeAddAnimalOnMap();
void AfterAddAnimalOnMap();
// CommonDll cScrCharacterEx::StartTask / SetTaskParam and GameDll 0x16300 CustomProps (script natives,
// this = script object): what a reserve copies from the script animal it joins.
void AfterStartTask(void* scr, const char* task);
void AfterSetTaskParam(void* scr, const char* key, const char* value);
void AfterCustomProps(void* scr, const char* method);
// GameDll 0x16a40 script Kill() (parks the animal): when the script parks every animal of a family, its reserves go too.
void AfterKill(void* scr);
// GameDll 0x16d20 MakeBoss(on, name) and 0x15ff0 SetHealth(hp) (this = script object).
void AfterMakeBoss(void* scr, int on);
void AfterSetHealth(void* scr);
// A reserve animal of this map (both peers).
bool IsReserve(void* ent);

}  // namespace dh2::coop::scale
