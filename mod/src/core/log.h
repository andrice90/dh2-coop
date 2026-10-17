#pragma once
// Logging (spdlog, file next to the ASI: dh2coop.log) and ASI self-location helpers.
#include <windows.h>

#include <filesystem>
#include <string>

#include <spdlog/spdlog.h>

namespace dh2::core {

// Called once from DllMain with the ASI's own HMODULE.
void set_self_module(HMODULE self);
HMODULE self_module();
std::filesystem::path self_path();  // full path of dh2coop.asi
std::filesystem::path self_dir();   // folder containing it (game\Bin)

// Folder for our logs: self_dir(), or %LOCALAPPDATA%\dh2coop when the game is under Program Files.
std::filesystem::path log_dir();
// Creates the default spdlog logger writing to <log_dir>\dh2coop.log. The two runs before are kept as
// dh2coop.1.log / dh2coop.2.log, and the engine's debug.log of the previous run as debug.1.log / debug.2.log.
// Every message is flushed immediately so the log survives a crash.
void log_init(const std::string& level);

}  // namespace dh2::core
