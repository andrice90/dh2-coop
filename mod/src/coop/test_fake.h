#pragma once
// Test-only sender overrides (console coop_fake*): change what this process reports about its OWN player so a
// second window sees a puppet in a known pose (weapon, view pitch/yaw, crouch, reload, shots, a spot, a walk).
// Nothing happens until a coop_fake* command is used. See re/notes/puppet-polish.md.
#include "net/protocol.h"

namespace dh2::coop::testfake {

// Called for every PlayerState this process sends (game thread).
void Apply(net::PlayerState& st);

}  // namespace dh2::coop::testfake
