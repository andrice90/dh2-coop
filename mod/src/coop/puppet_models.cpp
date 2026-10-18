#include "coop/puppet_models.h"

namespace dh2::coop {
namespace {

#define DH2_NPC(dir) "objects/npc/" dir "/" dir ".skin,game.ap,shotgun1.ap,handgun3.ap"

// Order is part of the protocol: append only.
constexpr PuppetModel kModels[] = {
    {"Guide", DH2_NPC("mountainguide")},
    {"Hugh", DH2_NPC("oldhugh")},
    {"Reggie", DH2_NPC("reginald")},
    {"Hamisi", DH2_NPC("hamisi")},
    {"Ranger", DH2_NPC("ausranger1")},
    {"Siberian", DH2_NPC("siberian1")},
    {"Wirake", DH2_NPC("wirake")},
    {"Tribal leader", DH2_NPC("triballeader")},
    {"Benedeck", DH2_NPC("benedeck")},
    {"Survivor", DH2_NPC("survivor")},
    // Not offered: poacher (HandleKey model_name faults on that skin, 2026-10-17), teengirl / nephew / fiance (no
    // idle clip), abby / indiangirl (female rig, untested).
};

#undef DH2_NPC

}  // namespace

int PuppetModelCount() { return static_cast<int>(sizeof kModels / sizeof kModels[0]); }

const PuppetModel& PuppetModelAt(int index) {
    if (index < 0 || index >= PuppetModelCount()) index = 0;
    return kModels[index];
}

}  // namespace dh2::coop
