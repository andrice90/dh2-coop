#pragma once
// Windows clipboard text (CF_UNICODETEXT) for the lobby's copy buttons. Win32 only, no engine calls.
#include <string>

namespace dh2::coop {

// Puts `utf8` on the clipboard as CF_UNICODETEXT (LF becomes CRLF). False when the clipboard stays busy.
// `owner` is a window of this process (the clipboard owner); null is accepted and works too.
bool SetClipboardUnicode(const std::string& utf8, void* owner = nullptr);
// Reads CF_UNICODETEXT back as UTF-8 (tests in their own window station only; the game never reads the clipboard).
bool GetClipboardUnicode(std::string& utf8, void* owner = nullptr);

// A visible top-level window of this process, or null.
void* ProcessMainWindow();

}  // namespace dh2::coop
