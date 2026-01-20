#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../global/Flags.h"
#include "../global/hash.h"
#include "../global/macros.h"

#include <cstdint>
#include <string>
#include <string_view>

#include <QString>
#include <Qt>

#if defined(_MSC_VER) || defined(__MINGW32__)
#undef DELETE  // Bad dog, Microsoft; bad dog!!!
#undef INVALID // Bad dog, Microsoft; bad dog!!!
#endif

// Macro to define all supporting hotkey policies.
// X(EnumName, Marker, Help)
#define XFOREACH_HOTKEY_POLICY(X) \
    X(Any, "", "Can be bound with or without modifiers (e.g. F-keys)") \
    X(Keypad, "", "Can be bound with or without modifiers (e.g. Numpad)") \
    X(ModifierRequired, "*", "Requires a modifier (CTRL, ALT, SHIFT, or META)") \
    X(ModifierNotShift, "**", "Requires a non-SHIFT modifier (CTRL, ALT, or META)")

enum class HotkeyPolicyEnum : uint8_t {
#define X_ENUM(name, marker, help) name,
    XFOREACH_HOTKEY_POLICY(X_ENUM)
#undef X_ENUM
};

#define X_COUNT(name, marker, help) +1
static constexpr const size_t NUM_HOTKEY_POLICIES = 0 XFOREACH_HOTKEY_POLICY(X_COUNT);
#undef X_COUNT
static_assert(NUM_HOTKEY_POLICIES == 4);

// Macro to define all supported base keys and their Qt mappings.
// X(EnumName, StringName, QtKey, Policy) -> Defines a unique identity
#define XFOREACH_HOTKEY_BASE_KEYS(X) \
    X(F1, "F1", Qt::Key_F1, HotkeyPolicyEnum::Any) \
    X(F2, "F2", Qt::Key_F2, HotkeyPolicyEnum::Any) \
    X(F3, "F3", Qt::Key_F3, HotkeyPolicyEnum::Any) \
    X(F4, "F4", Qt::Key_F4, HotkeyPolicyEnum::Any) \
    X(F5, "F5", Qt::Key_F5, HotkeyPolicyEnum::Any) \
    X(F6, "F6", Qt::Key_F6, HotkeyPolicyEnum::Any) \
    X(F7, "F7", Qt::Key_F7, HotkeyPolicyEnum::Any) \
    X(F8, "F8", Qt::Key_F8, HotkeyPolicyEnum::Any) \
    X(F9, "F9", Qt::Key_F9, HotkeyPolicyEnum::Any) \
    X(F10, "F10", Qt::Key_F10, HotkeyPolicyEnum::Any) \
    X(F11, "F11", Qt::Key_F11, HotkeyPolicyEnum::Any) \
    X(F12, "F12", Qt::Key_F12, HotkeyPolicyEnum::Any) \
    X(NUMPAD0, "NUMPAD0", Qt::Key_0, HotkeyPolicyEnum::Keypad) \
    X(NUMPAD1, "NUMPAD1", Qt::Key_1, HotkeyPolicyEnum::Keypad) \
    X(NUMPAD2, "NUMPAD2", Qt::Key_2, HotkeyPolicyEnum::Keypad) \
    X(NUMPAD3, "NUMPAD3", Qt::Key_3, HotkeyPolicyEnum::Keypad) \
    X(NUMPAD4, "NUMPAD4", Qt::Key_4, HotkeyPolicyEnum::Keypad) \
    X(NUMPAD5, "NUMPAD5", Qt::Key_5, HotkeyPolicyEnum::Keypad) \
    X(NUMPAD6, "NUMPAD6", Qt::Key_6, HotkeyPolicyEnum::Keypad) \
    X(NUMPAD7, "NUMPAD7", Qt::Key_7, HotkeyPolicyEnum::Keypad) \
    X(NUMPAD8, "NUMPAD8", Qt::Key_8, HotkeyPolicyEnum::Keypad) \
    X(NUMPAD9, "NUMPAD9", Qt::Key_9, HotkeyPolicyEnum::Keypad) \
    X(NUMPAD_SLASH, "NUMPAD_SLASH", Qt::Key_Slash, HotkeyPolicyEnum::Keypad) \
    X(NUMPAD_ASTERISK, "NUMPAD_ASTERISK", Qt::Key_Asterisk, HotkeyPolicyEnum::Keypad) \
    X(NUMPAD_MINUS, "NUMPAD_MINUS", Qt::Key_Minus, HotkeyPolicyEnum::Keypad) \
    X(NUMPAD_PLUS, "NUMPAD_PLUS", Qt::Key_Plus, HotkeyPolicyEnum::Keypad) \
    X(NUMPAD_PERIOD, "NUMPAD_PERIOD", Qt::Key_Period, HotkeyPolicyEnum::Keypad) \
    X(HOME, "HOME", Qt::Key_Home, HotkeyPolicyEnum::ModifierRequired) \
    X(END, "END", Qt::Key_End, HotkeyPolicyEnum::ModifierRequired) \
    X(INSERT, "INSERT", Qt::Key_Insert, HotkeyPolicyEnum::ModifierRequired) \
    X(PAGEUP, "PAGEUP", Qt::Key_PageUp, HotkeyPolicyEnum::ModifierRequired) \
    X(PAGEDOWN, "PAGEDOWN", Qt::Key_PageDown, HotkeyPolicyEnum::ModifierRequired) \
    X(UP, "UP", Qt::Key_Up, HotkeyPolicyEnum::ModifierRequired) \
    X(DOWN, "DOWN", Qt::Key_Down, HotkeyPolicyEnum::ModifierRequired) \
    X(LEFT, "LEFT", Qt::Key_Left, HotkeyPolicyEnum::ModifierRequired) \
    X(RIGHT, "RIGHT", Qt::Key_Right, HotkeyPolicyEnum::ModifierRequired) \
    X(DELETE, "DELETE", Qt::Key_Delete, HotkeyPolicyEnum::ModifierRequired) \
    X(ACCENT, "ACCENT", Qt::Key_QuoteLeft, HotkeyPolicyEnum::ModifierNotShift) \
    X(K_0, "0", Qt::Key_0, HotkeyPolicyEnum::ModifierNotShift) \
    X(K_1, "1", Qt::Key_1, HotkeyPolicyEnum::ModifierNotShift) \
    X(K_2, "2", Qt::Key_2, HotkeyPolicyEnum::ModifierNotShift) \
    X(K_3, "3", Qt::Key_3, HotkeyPolicyEnum::ModifierNotShift) \
    X(K_4, "4", Qt::Key_4, HotkeyPolicyEnum::ModifierNotShift) \
    X(K_5, "5", Qt::Key_5, HotkeyPolicyEnum::ModifierNotShift) \
    X(K_6, "6", Qt::Key_6, HotkeyPolicyEnum::ModifierNotShift) \
    X(K_7, "7", Qt::Key_7, HotkeyPolicyEnum::ModifierNotShift) \
    X(K_8, "8", Qt::Key_8, HotkeyPolicyEnum::ModifierNotShift) \
    X(K_9, "9", Qt::Key_9, HotkeyPolicyEnum::ModifierNotShift) \
    X(HYPHEN, "HYPHEN", Qt::Key_Minus, HotkeyPolicyEnum::ModifierNotShift) \
    X(EQUAL, "EQUAL", Qt::Key_Equal, HotkeyPolicyEnum::ModifierNotShift)

// Macro to define the mapping between keypad navigation keys and their numeric counterparts.
// X(From, To)
#define XFOREACH_HOTKEY_KEYPAD_MAP(X) \
    X(Qt::Key_Home, Qt::Key_7) \
    X(Qt::Key_Up, Qt::Key_8) \
    X(Qt::Key_PageUp, Qt::Key_9) \
    X(Qt::Key_Left, Qt::Key_4) \
    X(Qt::Key_Clear, Qt::Key_5) \
    X(Qt::Key_Right, Qt::Key_6) \
    X(Qt::Key_End, Qt::Key_1) \
    X(Qt::Key_Down, Qt::Key_2) \
    X(Qt::Key_PageDown, Qt::Key_3) \
    X(Qt::Key_Insert, Qt::Key_0) \
    X(Qt::Key_Delete, Qt::Key_Period)

#define X_COUNT(from, to) +1
static constexpr const size_t NUM_HOTKEY_KEYPAD_KEYS = 0 XFOREACH_HOTKEY_KEYPAD_MAP(X_COUNT);
#undef X_COUNT
static_assert(NUM_HOTKEY_KEYPAD_KEYS == 11);

enum class HotkeyEnum : uint8_t {
#define X_ENUM(id, name, key, pol) id,
    XFOREACH_HOTKEY_BASE_KEYS(X_ENUM)
#undef X_ENUM
        INVALID
};
#define X_COUNT(id, name, key, pol) +1
static constexpr const size_t NUM_HOTKEY_KEYS = 0 XFOREACH_HOTKEY_BASE_KEYS(X_COUNT);
#undef X_COUNT
static_assert(NUM_HOTKEY_KEYS == 50);

// Macro to define hotkeys modifiers
// X(UPPER, CamelCase, QtEnum)
#define XFOREACH_HOTKEY_MODIFIER(X) \
    X(SHIFT, Shift, Qt::ShiftModifier) \
    X(CTRL, Ctrl, Qt::ControlModifier) \
    X(ALT, Alt, Qt::AltModifier) \
    X(META, Meta, Qt::MetaModifier)

enum class HotkeyModifierEnum : uint8_t {
#define X_ENUM(id, camel, qtenum) id,
    XFOREACH_HOTKEY_MODIFIER(X_ENUM)
#undef X_ENUM
};

#define X_COUNT(id, camel, qtenum) +1
static constexpr const size_t NUM_HOTKEY_MODIFIERS = 0 XFOREACH_HOTKEY_MODIFIER(X_COUNT);
#undef X_COUNT
static_assert(NUM_HOTKEY_MODIFIERS == 4);
DEFINE_ENUM_COUNT(HotkeyModifierEnum, NUM_HOTKEY_MODIFIERS)

class NODISCARD HotkeyModifiers final
    : public enums::Flags<HotkeyModifiers, HotkeyModifierEnum, uint8_t>
{
public:
    using Flags::Flags;

public:
#define X_IS_POLICY(name, camel, qtenum) \
    NODISCARD bool is##camel() const { return contains(HotkeyModifierEnum::name); }
    XFOREACH_HOTKEY_MODIFIER(X_IS_POLICY)
#undef X_IS_POLICY
};

namespace {
constexpr bool isUppercase(const char *s)
{
    while (*s) {
        if (*s >= 'a' && *s <= 'z')
            return false;
        s++;
    }
    return true;
}
#define X_CHECK_UPPER(id, name, qkey, policy) \
    static_assert(isUppercase(name), "Hotkey name must be uppercase: " name);
XFOREACH_HOTKEY_BASE_KEYS(X_CHECK_UPPER)
#undef X_CHECK_UPPER
#define X_CHECK_UPPER(name, camel, qtenum) \
    static_assert(isUppercase(#name), "Hotkey modifier must be uppercase: " #name);
XFOREACH_HOTKEY_MODIFIER(X_CHECK_UPPER)
#undef X_CHECK_UPPER
} // namespace

class NODISCARD Hotkey final
{
private:
    HotkeyEnum m_base = HotkeyEnum::INVALID;
    HotkeyModifiers m_modifiers = {};
    HotkeyPolicyEnum m_policy = HotkeyPolicyEnum::Any;

public:
    Hotkey() = delete;
    explicit Hotkey(std::string_view s);
    explicit Hotkey(Qt::Key key, Qt::KeyboardModifiers modifiers);

public:
    NODISCARD bool isRecognized() const { return m_base != HotkeyEnum::INVALID; }
    NODISCARD bool isPolicySatisfied() const;
    NODISCARD bool isValid() const { return isRecognized() && isPolicySatisfied(); }

public:
    NODISCARD std::string to_string() const;

public:
    NODISCARD bool operator==(const Hotkey &other) const
    {
        return m_base == other.m_base && m_modifiers == other.m_modifiers;
    }
    NODISCARD bool operator!=(const Hotkey &other) const { return !(*this == other); }

public:
    NODISCARD HotkeyEnum base() const { return m_base; }
    NODISCARD HotkeyModifiers modifiers() const { return m_modifiers; }
    NODISCARD HotkeyPolicyEnum policy() const { return m_policy; }

public:
#define X_IS_POLICY(name, marker, help) \
    NODISCARD bool is##name() const { return m_policy == HotkeyPolicyEnum::name; }
    XFOREACH_HOTKEY_POLICY(X_IS_POLICY)
#undef X_IS_POLICY
};

template<>
struct std::hash<Hotkey>
{
    std::size_t operator()(const Hotkey &hk) const noexcept
    {
        size_t seed = 0;
        hash_combine(seed, static_cast<uint8_t>(hk.base()));
        hash_combine(seed, hk.modifiers().asUint32());
        return seed;
    }
};
