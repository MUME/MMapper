#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../global/ChangeMonitor.h"
#include "../global/RuleOf5.h"
#include "../global/macros.h"
#include "Hotkey.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <QString>
#include <Qt>

// Macro to define default hotkeys
// X(SerializedKey, Command)
#define XFOREACH_DEFAULT_HOTKEYS(X) \
    X("F1", "F1") \
    X("F2", "F2") \
    X("F3", "F3") \
    X("F4", "F4") \
    X("F5", "F5") \
    X("F6", "F6") \
    X("F7", "F7") \
    X("F8", "F8") \
    X("F9", "F9") \
    X("F10", "F10") \
    X("F11", "F11") \
    X("F12", "F12") \
    X("NUMPAD8", "north") \
    X("NUMPAD4", "west") \
    X("NUMPAD6", "east") \
    X("NUMPAD5", "south") \
    X("NUMPAD_MINUS", "up") \
    X("NUMPAD_PLUS", "down") \
    X("CTRL+NUMPAD8", "open exit north") \
    X("CTRL+NUMPAD4", "open exit west") \
    X("CTRL+NUMPAD6", "open exit east") \
    X("CTRL+NUMPAD5", "open exit south") \
    X("CTRL+NUMPAD_MINUS", "open exit up") \
    X("CTRL+NUMPAD_PLUS", "open exit down") \
    X("ALT+NUMPAD8", "close exit north") \
    X("ALT+NUMPAD4", "close exit west") \
    X("ALT+NUMPAD6", "close exit east") \
    X("ALT+NUMPAD5", "close exit south") \
    X("ALT+NUMPAD_MINUS", "close exit up") \
    X("ALT+NUMPAD_PLUS", "close exit down") \
    X("SHIFT+NUMPAD8", "pick exit north") \
    X("SHIFT+NUMPAD4", "pick exit west") \
    X("SHIFT+NUMPAD6", "pick exit east") \
    X("SHIFT+NUMPAD5", "pick exit south") \
    X("SHIFT+NUMPAD_MINUS", "pick exit up") \
    X("SHIFT+NUMPAD_PLUS", "pick exit down") \
    X("NUMPAD7", "look") \
    X("NUMPAD9", "flee") \
    X("NUMPAD2", "lead") \
    X("NUMPAD0", "bash") \
    X("NUMPAD1", "ride") \
    X("NUMPAD3", "stand")

namespace {

constexpr bool is_valid_hotkey(std::string_view hotkey_str)
{
    // Find the base key (the part after the last '+')
    size_t last_plus = hotkey_str.rfind('+');
    std::string_view base_part = (last_plus == std::string_view::npos)
                                     ? hotkey_str
                                     : hotkey_str.substr(last_plus + 1);

    // Determine which modifiers are present
    bool has_ctrl = hotkey_str.find("CTRL") != std::string_view::npos;
    bool has_alt = hotkey_str.find("ALT") != std::string_view::npos;
    bool has_shift = hotkey_str.find("SHIFT") != std::string_view::npos;
    bool has_meta = hotkey_str.find("META") != std::string_view::npos;
    bool has_any_mod = has_ctrl || has_alt || has_shift || has_meta;

    // Match against the base key and check policy
#define CHECK_POLICY(id, name, key, policy) \
    if (base_part == name) { \
        if (policy == HotkeyPolicyEnum::ModifierRequired) \
            return has_any_mod; \
        if (policy == HotkeyPolicyEnum::ModifierNotShift) \
            return (has_ctrl || has_alt || has_meta); \
        return true; \
    }

    XFOREACH_HOTKEY_BASE_KEYS(CHECK_POLICY)
#undef CHECK_POLICY

    // Key name not found
    return false;
}

// This template trick ensures the compiler evaluates the expression for every macro entry
template<bool V>
struct Validate
{
    static_assert(V, "Hotkey policy violation detected!");
};

#define APPLY_VALIDATION(SerializedKey, Command) \
    static_assert(is_valid_hotkey(SerializedKey), "Invalid Hotkey Policy for: " SerializedKey);
XFOREACH_DEFAULT_HOTKEYS(APPLY_VALIDATION)
#undef APPLY_VALIDATION

} // namespace

class NODISCARD HotkeyManager final
{
private:
    std::unordered_map<Hotkey, std::string> m_hotkeys;
    ChangeMonitor::Lifetime m_configLifetime;

public:
    HotkeyManager();
    ~HotkeyManager() = default;

    DELETE_CTORS_AND_ASSIGN_OPS(HotkeyManager);

private:
    void syncFromConfig();

public:
    NODISCARD bool setHotkey(const Hotkey &hk, std::string command);
    NODISCARD bool removeHotkey(const Hotkey &hk);

public:
    NODISCARD std::optional<std::string> getCommand(const Hotkey &hk) const;
    NODISCARD std::vector<std::pair<Hotkey, std::string>> getAllHotkeys() const;

public:
    void resetToDefaults();
    void clear();
};
