#include "core/log.h"

#include <spdlog/sinks/basic_file_sink.h>

namespace dh2::core {

static HMODULE g_self = nullptr;

void set_self_module(HMODULE self) { g_self = self; }
HMODULE self_module() { return g_self; }

std::filesystem::path self_path() {
    wchar_t buf[MAX_PATH * 2] = {};
    DWORD n = GetModuleFileNameW(g_self, buf, static_cast<DWORD>(std::size(buf)));
    return std::filesystem::path(std::wstring(buf, n));
}

std::filesystem::path self_dir() { return self_path().parent_path(); }

void log_init(const std::string& level) {
    try {
        auto path = self_dir() / "dh2coop.log";
        auto logger = spdlog::basic_logger_mt("dh2coop", path.string(), /*truncate=*/true);
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [t%t] %v");
        spdlog::set_default_logger(logger);
        spdlog::set_level(spdlog::level::from_str(level));
        spdlog::flush_on(spdlog::level::trace);  // flush every message
    } catch (const spdlog::spdlog_ex&) {
        // No logging available; the mod keeps running silently.
    }
}

}  // namespace dh2::core
