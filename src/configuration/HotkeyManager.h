#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../global/RuleOf5.h"
#include "../global/macros.h"

#include <vector>

#include <QHash>
#include <QString>
#include <QStringList>

class NODISCARD HotkeyManager final
{
private:
    // Fast lookup map for runtime hotkey resolution
    QHash<QString, QString> m_hotkeys;

    // Ordered list of hotkey entries (key, command) to preserve user's order
    std::vector<std::pair<QString, QString>> m_orderedHotkeys;

    // Raw content preserving comments and formatting (used for export)
    QString m_rawContent;

    /// Normalize a key string to canonical modifier order: CTRL+SHIFT+ALT+META+Key
    /// Example: "ALT+CTRL+F1" -> "CTRL+ALT+F1"
    NODISCARD static QString normalizeKeyString(const QString &keyString);

    /// Parse raw content to populate m_hotkeys and m_orderedHotkeys
    void parseRawContent();

public:
    HotkeyManager();
    ~HotkeyManager() = default;

    DELETE_CTORS_AND_ASSIGN_OPS(HotkeyManager);

    /// Load hotkeys from QSettings (called on startup)
    void loadFromSettings();

    /// Save hotkeys to QSettings
    void saveToSettings() const;

    /// Set a hotkey (saves to QSettings immediately)
    void setHotkey(const QString &keyName, const QString &command);

    /// Remove a hotkey (saves to QSettings immediately)
    void removeHotkey(const QString &keyName);

    /// Get the command for a given key name (e.g., "F1", "CTRL+F1")
    /// Returns empty string if no hotkey is configured
    NODISCARD QString getCommand(const QString &keyName) const;

    /// Check if a hotkey is configured for the given key
    NODISCARD bool hasHotkey(const QString &keyName) const;

    /// Get all configured hotkeys in their original order
    NODISCARD const std::vector<std::pair<QString, QString>> &getAllHotkeys() const
    {
        return m_orderedHotkeys;
    }

    /// Reset hotkeys to defaults (clears all and loads defaults)
    void resetToDefaults();

    /// Export hotkeys to CLI command format (for _config edit and export)
    NODISCARD QString exportToCliFormat() const;

    /// Import hotkeys from CLI command format (clears existing hotkeys first)
    /// Returns the number of hotkeys imported
    int importFromCliFormat(const QString &content);

    /// Get list of available key names for _hotkey keys command
    NODISCARD static QStringList getAvailableKeyNames();

    /// Get list of available modifiers for _hotkey keys command
    NODISCARD static QStringList getAvailableModifiers();
};
