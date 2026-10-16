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

// Creates the default spdlog logger writing to <self_dir>\dh2coop.log (truncated per run).
// Every message is flushed immediately so the log survives a crash.
void log_init(const std::string& level);

}  // namespace dh2::core
