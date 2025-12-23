#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../global/RuleOf5.h"
#include "../global/macros.h"

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include <QString>
#include <Qt>

/// Represents a hotkey as (key, modifiers, isNumpad) for efficient lookup
struct NODISCARD HotkeyKey final
{
    int key = 0;
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;
    bool isNumpad = false; // true if this is a numpad key (NUMPAD0-9, NUMPAD_MINUS, etc.)

    HotkeyKey() = default;
    HotkeyKey(int k, Qt::KeyboardModifiers m, bool numpad = false)
        : key(k)
        , modifiers(m)
        , isNumpad(numpad)
    {}

    NODISCARD bool operator==(const HotkeyKey &other) const
    {
        return key == other.key && modifiers == other.modifiers && isNumpad == other.isNumpad;
    }
};

/// Hash function for HotkeyKey to use in std::unordered_map
struct HotkeyKeyHash final
{
    NODISCARD std::size_t operator()(const HotkeyKey &k) const noexcept
    {
        // Combine hash values using XOR and bit shifting
        std::size_t h1 = std::hash<int>{}(k.key);
        std::size_t h2 = std::hash<int>{}(static_cast<int>(k.modifiers));
        std::size_t h3 = std::hash<bool>{}(k.isNumpad);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

class NODISCARD HotkeyManager final
{
private:
    // Fast lookup map for runtime hotkey resolution: (key, modifiers) -> command (std::string)
    std::unordered_map<HotkeyKey, std::string, HotkeyKeyHash> m_hotkeys;

    // Ordered list of hotkey entries (key string, command) to preserve user's order for display
    std::vector<std::pair<QString, std::string>> m_orderedHotkeys;

    // Raw content preserving comments and formatting (used for export)
    QString m_rawContent;

    /// Normalize a key string to canonical modifier order: CTRL+SHIFT+ALT+META+Key
    /// Example: "ALT+CTRL+F1" -> "CTRL+ALT+F1"
    /// Returns empty string if the base key is invalid
    NODISCARD static QString normalizeKeyString(const QString &keyString);

    /// Check if a base key name (without modifiers) is valid
    NODISCARD static bool isValidBaseKey(const QString &baseKey);

    /// Parse raw content to populate m_hotkeys and m_orderedHotkeys
    void parseRawContent();

    /// Convert a key string (e.g., "CTRL+F1") to a HotkeyKey
    /// Returns HotkeyKey with key=0 if parsing fails
    NODISCARD static HotkeyKey stringToHotkeyKey(const QString &keyString);

    /// Convert a HotkeyKey to a normalized key string (e.g., "CTRL+F1")
    NODISCARD static QString hotkeyKeyToString(const HotkeyKey &hk);

    /// Convert a base key name (e.g., "F1", "NUMPAD8") to Qt::Key
    /// Returns 0 if the key name is not recognized
    NODISCARD static int baseKeyNameToQtKey(const QString &keyName);

    /// Convert a Qt::Key to a base key name (e.g., Qt::Key_F1 -> "F1")
    /// Returns empty string if the key is not recognized
    NODISCARD static QString qtKeyToBaseKeyName(int qtKey);

public:
    HotkeyManager();
    ~HotkeyManager() = default;

    DELETE_CTORS_AND_ASSIGN_OPS(HotkeyManager);

    /// Load hotkeys from QSettings (called on startup)
    void loadFromSettings();

    /// Save hotkeys to QSettings
    void saveToSettings() const;

    /// Set a hotkey using string key name (saves to QSettings immediately)
    /// This is used by the _hotkey command for user convenience
    /// Returns true if the hotkey was set successfully, false if the key name is invalid
    NODISCARD bool setHotkey(const QString &keyName, const QString &command);

    /// Remove a hotkey using string key name (saves to QSettings immediately)
    void removeHotkey(const QString &keyName);

    /// Get the command for a given key and modifiers (optimized for runtime lookup)
    /// isNumpad should be true if the key was pressed on the numpad (KeypadModifier was set)
    /// Returns empty string if no hotkey is configured
    NODISCARD std::string getCommand(int key, Qt::KeyboardModifiers modifiers, bool isNumpad) const;

    /// Get the command for a given key name string (for _hotkey command)
    /// Returns empty string if no hotkey is configured
    NODISCARD std::string getCommand(const QString &keyName) const;

    /// Convenience: Get command as QString (for Qt UI layer)
    /// Returns empty QString if no hotkey is configured
    NODISCARD QString getCommandQString(int key,
                                        Qt::KeyboardModifiers modifiers,
                                        bool isNumpad) const;

    /// Convenience: Get command as QString by key name (for Qt UI layer)
    /// Returns empty QString if no hotkey is configured
    NODISCARD QString getCommandQString(const QString &keyName) const;

    /// Check if a hotkey is configured for the given key name
    NODISCARD bool hasHotkey(const QString &keyName) const;

    /// Get all configured hotkeys in their original order (key string, command as std::string)
    NODISCARD const std::vector<std::pair<QString, std::string>> &getAllHotkeys() const
    {
        return m_orderedHotkeys;
    }

    /// Reset hotkeys to defaults (clears all and loads defaults)
    void resetToDefaults();

    /// Clear all hotkeys (does not save to settings)
    void clear();

    /// Get all key names that have hotkeys configured
    NODISCARD std::vector<QString> getAllKeyNames() const;

    /// Export hotkeys to CLI command format (for _config edit and export)
    NODISCARD QString exportToCliFormat() const;

    /// Import hotkeys from CLI command format (clears existing hotkeys first)
    /// Returns the number of hotkeys imported
    int importFromCliFormat(const QString &content);

    /// Get list of available key names for _hotkey keys command
    NODISCARD static std::vector<QString> getAvailableKeyNames();

    /// Get list of available modifiers for _hotkey keys command
    NODISCARD static std::vector<QString> getAvailableModifiers();
};
