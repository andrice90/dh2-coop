#include "core/hook.h"

#include <spdlog/spdlog.h>

#include "core/exports.h"

namespace dh2::core {

safetyhook::InlineHook* install_hook(const char* label, void* target, void* detour) {
    if (!target || !detour) {
        spdlog::warn("hook {}: target not resolved, skipped", label);
        return nullptr;
    }
    auto created = safetyhook::InlineHook::create(target, detour);
    if (!created) {
        spdlog::error("hook {}: create failed (safetyhook error type {}) target={}", label,
                      static_cast<int>(created.error().type), describe_address(target));
        return nullptr;
    }
    auto* hook = new safetyhook::InlineHook(std::move(*created));
    spdlog::info("hook {}: installed at {} ({:#x})", label, describe_address(target),
                 reinterpret_cast<uintptr_t>(target));
    return hook;
}

}  // namespace dh2::core
