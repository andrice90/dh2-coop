#include "core/log.h"

#include <cstdlib>
#include <system_error>

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

namespace {

bool under(const std::filesystem::path& p, const wchar_t* env) {
    const wchar_t* root = _wgetenv(env);
    if (!root || !*root) return false;
    std::wstring a = p.wstring(), b = root;
    for (auto& c : a) c = static_cast<wchar_t>(towlower(c));
    for (auto& c : b) c = static_cast<wchar_t>(towlower(c));
    return a.rfind(b, 0) == 0;
}

// name.log -> name.1.log -> name.2.log (the oldest is dropped). A crash and a relaunch keep the crashed run.
void rotate(const std::filesystem::path& dir, const std::string& stem, bool copy_current) {
    std::error_code ec;
    const auto cur = dir / (stem + ".log");
    const auto one = dir / (stem + ".1.log");
    const auto two = dir / (stem + ".2.log");
    if (!std::filesystem::exists(cur, ec)) return;
    std::filesystem::remove(two, ec);
    if (std::filesystem::exists(one, ec)) std::filesystem::rename(one, two, ec);
    if (copy_current) std::filesystem::copy_file(cur, one, std::filesystem::copy_options::overwrite_existing, ec);
    else std::filesystem::rename(cur, one, ec);
}

}  // namespace

std::filesystem::path log_dir() {
    // A game under Program Files is not writable for the player: Windows either refuses or silently redirects the
    // file to %LOCALAPPDATA%\VirtualStore (two-PC test 2026-10-17: "PC2 didn't save a dh2coop.log"). Use a folder
    // the player can find instead.
    const auto dir = self_dir();
    if (under(dir, L"ProgramFiles") || under(dir, L"ProgramFiles(x86)")) {
        if (const wchar_t* local = _wgetenv(L"LOCALAPPDATA")) {
            std::error_code ec;
            auto out = std::filesystem::path(local) / L"dh2coop";
            std::filesystem::create_directories(out, ec);
            if (!ec) return out;
        }
    }
    return dir;
}

void keep_engine_log() {
    // The engine truncates its debug.log next to Main7.exe early in startup; keep the previous run's copy beside ours
    // (copy, never move: the engine's own file stays where it is). Done from init_thread this raced the engine and
    // kept only the new run's first five lines (log sweep 2026-10-17), so DllMain calls this. Plain file calls only.
    const auto dir = log_dir();
    const auto engine_log = self_dir() / L"debug.log";
    if (GetFileAttributesW(engine_log.c_str()) == INVALID_FILE_ATTRIBUTES) return;
    const auto one = dir / L"debug.1.log";
    const auto two = dir / L"debug.2.log";
    DeleteFileW(two.c_str());
    MoveFileW(one.c_str(), two.c_str());
    CopyFileW(engine_log.c_str(), one.c_str(), FALSE);
}

void log_init(const std::string& level) {
    try {
        const auto dir = log_dir();
        rotate(dir, "dh2coop", false);
        auto path = dir / "dh2coop.log";
        auto logger = spdlog::basic_logger_mt("dh2coop", path.string(), /*truncate=*/true);
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [t%t] %v");
        spdlog::set_default_logger(logger);
        spdlog::set_level(spdlog::level::from_str(level));
        spdlog::flush_on(spdlog::level::trace);  // flush every message
        spdlog::info("log: {} (previous runs: dh2coop.1.log, dh2coop.2.log, debug.1.log)", path.string());
    } catch (const spdlog::spdlog_ex&) {
        // No logging available; the mod keeps running silently.
    }
}

}  // namespace dh2::core
