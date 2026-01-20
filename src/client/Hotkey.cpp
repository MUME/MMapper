// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "Hotkey.h"

#include "../global/CaseUtils.h"
#include "../global/CharUtils.h"
#include "../global/ConfigConsts-Computed.h"
#include "../global/TextUtils.h"

#include <sstream>

bool Hotkey::isPolicySatisfied() const
{
    const auto mods = modifiers();
    if (isModifierRequired() && mods.isEmpty()) {
        return false;
    }
    if (isModifierNotShift()) {
        const bool isNoMod = mods.isEmpty();
        const bool isOnlyShift = (mods.isShift() && mods.size() == 1);
        if (isNoMod || isOnlyShift) {
            return false;
        }
    }
    return true;
}

Hotkey::Hotkey(std::string_view s)
{
    if (s.empty()) {
        return;
    }

    size_t start = 0;
    size_t end = s.find('+');
    while (true) {
        std::string part = std::invoke([&s, &start, &end]() {
            std::string_view sv = (end == std::string::npos) ? s.substr(start)
                                                             : s.substr(start, end - start);
            // Trim whitespace
            while (!sv.empty() && ascii::isSpace(sv.front())) {
                sv.remove_prefix(1);
            }
            while (!sv.empty() && ascii::isSpace(sv.back())) {
                sv.remove_suffix(1);
            }

            return ::toUpperUtf8(sv);
        });

        if (!part.empty()) {
#define X_PARSE(id, camel, qtenum) \
    if (part == #id) { \
        m_modifiers.insert(HotkeyModifierEnum::id); \
    } else
            XFOREACH_HOTKEY_MODIFIER(X_PARSE)
#undef X_PARSE
            {
                // Not a modifier, check if its a valid base key
#define X_NAME_TO_ENUM(id, str, qkey, pol) \
    if (part == str) { \
        m_base = HotkeyEnum::id; \
        m_policy = pol; \
    } else
                XFOREACH_HOTKEY_BASE_KEYS(X_NAME_TO_ENUM)
#undef X_NAME_TO_ENUM
                {
                    // Not a valid token
                    m_base = HotkeyEnum::INVALID;
                    m_modifiers.clear();
                    return;
                }
            }
        }

        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
        end = s.find('+', start);
    }
}

Hotkey::Hotkey(Qt::Key key, Qt::KeyboardModifiers modifiers)
{
    bool isNumpad = (modifiers & Qt::KeypadModifier);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
    if (isNumpad) {
        // Map keypad navigation keys to their numeric counterparts when isNumpad is true,
        // except on Mac where num lock does not exist.
        switch (key) {
#define X_KEYPAD(from, to) \
    case from: \
        if constexpr (CURRENT_PLATFORM == PlatformEnum::Mac) { \
            isNumpad = false; \
        } else { \
            key = to; \
        } \
        break;
            XFOREACH_HOTKEY_KEYPAD_MAP(X_KEYPAD)
#undef X_KEYPAD
        default:
            break;
        }
    }
#pragma clang diagnostic pop

    assert(m_base == HotkeyEnum::INVALID);
#define X_BASE(id, name, qk, pol) \
    if (key == qk && ((pol == HotkeyPolicyEnum::Keypad) == isNumpad)) { \
        m_base = HotkeyEnum::id; \
        m_policy = pol; \
    }
    XFOREACH_HOTKEY_BASE_KEYS(X_BASE)
#undef X_BASE
    if (m_base == HotkeyEnum::INVALID) {
        return;
    }

#define X_MODIFIER(id, camel, qtenum) \
    if (modifiers & qtenum) { \
        m_modifiers.insert(HotkeyModifierEnum::id); \
    }
    XFOREACH_HOTKEY_MODIFIER(X_MODIFIER)
#undef X_MODIFIER
}

std::string Hotkey::to_string() const
{
    std::stringstream oss;
#define X_STR(id, camel, qtenum) \
    if (modifiers().is##camel()) { \
        oss << #id << "+"; \
    }
    XFOREACH_HOTKEY_MODIFIER(X_STR)
#undef X_STR

    switch (base()) {
#define X_SERIALIZE(id, name, qkey, policy) \
    case HotkeyEnum::id: \
        oss << name; \
        break;
        XFOREACH_HOTKEY_BASE_KEYS(X_SERIALIZE)
#undef X_SERIALIZE
    case HotkeyEnum::INVALID:
        return {};
    }
    return oss.str();
}
