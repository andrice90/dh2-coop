#pragma once
// safetyhook install helper. Hooks are leaked on purpose: unhooking at process exit races the game.
#include <safetyhook.hpp>

namespace dh2::core {

safetyhook::InlineHook* install_hook(const char* label, void* target, void* detour);

}  // namespace dh2::core
