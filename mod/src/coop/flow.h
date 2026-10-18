#pragma once
// Mission flow for every player (re/notes/flow.md): level exits are the host's, mission messages that do not pass
// cMission (a guide's death, a level exit, the task status report) still reach every player once and only from the
// host, the host's results/pause menu choice (continue, retry, quit, the end-of-game movie) drives the clients,
// ammo-gated tasks fail only when no living player has ammo, and a client's revival resets its mission status.
#include <string>

namespace dh2::coop::flow {

// One line for the overlay ("Waiting for the host to choose", ...). "" when there is nothing to say.
std::string Banner();

// Client, events::OnMapStartCommit: the host announced `map`. True when this client is already on its way there
// because it followed the host's continue (kLevelNext): the announcement must not start a second load.
bool AlreadyFollowing(const std::string& map);

// Client, before it loads the host's map from a results or pause menu: clear the HUD's results-menu code (it would
// open the menu again before the map changes) and close the game menu.
void BeforeFollow();

// Client: events::ApplyMissionEnd is about to replay a host mission message with this code (5, 6, 7). The HUD shows
// that one; this client's own mission messages are dropped while online (the host decides).
void AllowMissionMessage(int code);

}  // namespace dh2::coop::flow
