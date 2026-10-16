#pragma once
// Runtime probe: records which renderer / modules / engine entry points the game really uses.
// Everything is pass-through (hooks only log); results land in dh2coop.log.
#include "core/config.h"

namespace dh2::probe {

// Blocking; call from the mod's init thread (never from DllMain).
void run(const dh2::core::Config& cfg);

}  // namespace dh2::probe
