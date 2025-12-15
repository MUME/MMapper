// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "HotkeyManager.h"

#include <QDebug>
#include <QRegularExpression>
#include <QSettings>
#include <QTextStream>

namespace {
constexpr const char *SETTINGS_GROUP = "IntegratedClient/Hotkeys";
constexpr const char *SETTINGS_RAW_CONTENT_KEY = "IntegratedClient/HotkeysRawContent";

// Default hotkeys content preserving order and formatting
const QString DEFAULT_HOTKEYS_CONTENT = R"(# Hotkey Configuration
# Format: _hotkey KEY command
# Lines starting with # are comments.

# Basic movement (numpad)
_hotkey NUMPAD8 n
_hotkey NUMPAD4 w
_hotkey NUMPAD6 e
_hotkey NUMPAD5 s
_hotkey NUMPAD_MINUS u
_hotkey NUMPAD_PLUS d

# Open exit (CTRL+numpad)
_hotkey CTRL+NUMPAD8 open exit n
_hotkey CTRL+NUMPAD4 open exit w
_hotkey CTRL+NUMPAD6 open exit e
_hotkey CTRL+NUMPAD5 open exit s
_hotkey CTRL+NUMPAD_MINUS open exit u
_hotkey CTRL+NUMPAD_PLUS open exit d

# Close exit (ALT+numpad)
_hotkey ALT+NUMPAD8 close exit n
_hotkey ALT+NUMPAD4 close exit w
_hotkey ALT+NUMPAD6 close exit e
_hotkey ALT+NUMPAD5 close exit s
_hotkey ALT+NUMPAD_MINUS close exit u
_hotkey ALT+NUMPAD_PLUS close exit d

# Pick exit (SHIFT+numpad)
_hotkey SHIFT+NUMPAD8 pick exit n
_hotkey SHIFT+NUMPAD4 pick exit w
_hotkey SHIFT+NUMPAD6 pick exit e
_hotkey SHIFT+NUMPAD5 pick exit s
_hotkey SHIFT+NUMPAD_MINUS pick exit u
_hotkey SHIFT+NUMPAD_PLUS pick exit d

# Other actions
_hotkey NUMPAD7 look
_hotkey NUMPAD9 flee
_hotkey NUMPAD2 lead
_hotkey NUMPAD0 bash
_hotkey NUMPAD1 ride
_hotkey NUMPAD3 stand
)";
} // namespace

HotkeyManager::HotkeyManager()
{
    loadFromSettings();
}

void HotkeyManager::loadFromSettings()
{
    m_hotkeys.clear();
    m_orderedHotkeys.clear();

    QSettings settings;

    // Try to load raw content first (preserves comments and order)
    m_rawContent = settings.value(SETTINGS_RAW_CONTENT_KEY).toString();

    if (m_rawContent.isEmpty()) {
        // Check if there are legacy hotkeys in the old format
        settings.beginGroup(SETTINGS_GROUP);
        const QStringList keys = settings.childKeys();
        settings.endGroup();

        if (keys.isEmpty()) {
            // First run - use default hotkeys
            m_rawContent = DEFAULT_HOTKEYS_CONTENT;
        } else {
            // Migrate from legacy format: build raw content from existing keys
            QString migrated;
            QTextStream stream(&migrated);
            stream << "# Hotkey Configuration\n";
            stream << "# Format: _hotkey KEY command\n\n";

            settings.beginGroup(SETTINGS_GROUP);
            for (const QString &key : keys) {
                QString command = settings.value(key).toString();
                if (!command.isEmpty()) {
                    stream << "_hotkey " << key << " " << command << "\n";
                }
            }
            settings.endGroup();
            m_rawContent = migrated;
        }
        // Save in new format
        saveToSettings();
    }

    // Parse the raw content to populate lookup structures
    parseRawContent();
}

void HotkeyManager::parseRawContent()
{
    // Regex for parsing _hotkey commands: _hotkey KEY command
    static const QRegularExpression hotkeyRegex(R"(^\s*_hotkey\s+(\S+)\s+(.+)$)");

    m_hotkeys.clear();
    m_orderedHotkeys.clear();

    const QStringList lines = m_rawContent.split('\n');

    for (const QString &line : lines) {
        QString trimmedLine = line.trimmed();

        // Skip empty lines and comments
        if (trimmedLine.isEmpty() || trimmedLine.startsWith('#')) {
            continue;
        }

        // Parse hotkey command
        QRegularExpressionMatch match = hotkeyRegex.match(trimmedLine);
        if (match.hasMatch()) {
            QString key = normalizeKeyString(match.captured(1));
            QString command = match.captured(2).trimmed();
            if (!key.isEmpty() && !command.isEmpty()) {
                m_hotkeys[key] = command;
                m_orderedHotkeys.emplace_back(key, command);
            }
        }
    }
}

void HotkeyManager::saveToSettings() const
{
    QSettings settings;

    // Remove legacy format if it exists
    settings.remove(SETTINGS_GROUP);

    // Save the raw content (preserves comments, order, and formatting)
    settings.setValue(SETTINGS_RAW_CONTENT_KEY, m_rawContent);
}

void HotkeyManager::setHotkey(const QString &keyName, const QString &command)
{
    QString normalizedKey = normalizeKeyString(keyName);
    if (normalizedKey.isEmpty()) {
        return;
    }

    // Update or add in raw content
    static const QRegularExpression hotkeyLineRegex(
        R"(^(\s*_hotkey\s+)(\S+)(\s+)(.+)$)",
        QRegularExpression::MultilineOption);

    QString newLine = "_hotkey " + normalizedKey + " " + command;
    bool found = false;

    // Try to find and replace existing hotkey line
    QStringList lines = m_rawContent.split('\n');
    for (int i = 0; i < lines.size(); ++i) {
        QRegularExpressionMatch match = hotkeyLineRegex.match(lines[i]);
        if (match.hasMatch()) {
            QString existingKey = normalizeKeyString(match.captured(2));
            if (existingKey == normalizedKey) {
                lines[i] = newLine;
                found = true;
                break;
            }
        }
    }

    if (!found) {
        // Append new hotkey at the end
        if (!m_rawContent.endsWith('\n')) {
            m_rawContent += '\n';
        }
        m_rawContent += newLine + '\n';
    } else {
        m_rawContent = lines.join('\n');
    }

    // Re-parse and save
    parseRawContent();
    saveToSettings();
}

void HotkeyManager::removeHotkey(const QString &keyName)
{
    QString normalizedKey = normalizeKeyString(keyName);
    if (normalizedKey.isEmpty()) {
        return;
    }

    if (!m_hotkeys.contains(normalizedKey)) {
        return;
    }

    // Remove from raw content
    static const QRegularExpression hotkeyLineRegex(R"(^\s*_hotkey\s+(\S+)\s+.+$)");

    QStringList lines = m_rawContent.split('\n');
    QStringList newLines;

    for (const QString &line : lines) {
        QRegularExpressionMatch match = hotkeyLineRegex.match(line);
        if (match.hasMatch()) {
            QString existingKey = normalizeKeyString(match.captured(1));
            if (existingKey == normalizedKey) {
                // Skip this line (remove it)
                continue;
            }
        }
        newLines.append(line);
    }

    m_rawContent = newLines.join('\n');

    // Re-parse and save
    parseRawContent();
    saveToSettings();
}

QString HotkeyManager::getCommand(const QString &keyName) const
{
    QString normalizedKey = normalizeKeyString(keyName);

    if (m_hotkeys.contains(normalizedKey)) {
        return m_hotkeys[normalizedKey];
    }

    return QString();
}

bool HotkeyManager::hasHotkey(const QString &keyName) const
{
    QString normalizedKey = normalizeKeyString(keyName);
    return m_hotkeys.contains(normalizedKey);
}

QString HotkeyManager::normalizeKeyString(const QString &keyString)
{
    // Split by '+' to get individual parts
    QStringList parts = keyString.split('+', Qt::SkipEmptyParts);

    if (parts.isEmpty()) {
        return QString();
    }

    // The last part is always the base key (e.g., F1, F2)
    QString baseKey = parts.last();
    parts.removeLast();

    // Build canonical order: CTRL, SHIFT, ALT, META
    QStringList normalizedParts;

    bool hasCtrl = false;
    bool hasShift = false;
    bool hasAlt = false;
    bool hasMeta = false;

    // Check which modifiers are present
    for (const QString &part : parts) {
        QString upperPart = part.toUpper().trimmed();
        if (upperPart == "CTRL" || upperPart == "CONTROL") {
            hasCtrl = true;
        } else if (upperPart == "SHIFT") {
            hasShift = true;
        } else if (upperPart == "ALT") {
            hasAlt = true;
        } else if (upperPart == "META" || upperPart == "CMD" || upperPart == "COMMAND") {
            hasMeta = true;
        }
    }

    // Add modifiers in canonical order
    if (hasCtrl) {
        normalizedParts << "CTRL";
    }
    if (hasShift) {
        normalizedParts << "SHIFT";
    }
    if (hasAlt) {
        normalizedParts << "ALT";
    }
    if (hasMeta) {
        normalizedParts << "META";
    }

    // Add the base key
    normalizedParts << baseKey.toUpper();

    return normalizedParts.join("+");
}

void HotkeyManager::resetToDefaults()
{
    m_rawContent = DEFAULT_HOTKEYS_CONTENT;
    parseRawContent();
    saveToSettings();
}

QString HotkeyManager::exportToCliFormat() const
{
    // Return the raw content exactly as saved (preserves order, comments, and formatting)
    return m_rawContent;
}

int HotkeyManager::importFromCliFormat(const QString &content)
{
    // Store the raw content exactly as provided (preserves order, comments, and formatting)
    m_rawContent = content;

    // Parse to populate lookup structures
    parseRawContent();

    // Save to settings
    saveToSettings();

    return static_cast<int>(m_orderedHotkeys.size());
}

QStringList HotkeyManager::getAvailableKeyNames()
{
    return QStringList{
        // Function keys
        "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
        // Numpad
        "NUMPAD0", "NUMPAD1", "NUMPAD2", "NUMPAD3", "NUMPAD4",
        "NUMPAD5", "NUMPAD6", "NUMPAD7", "NUMPAD8", "NUMPAD9",
        "NUMPAD_SLASH", "NUMPAD_ASTERISK", "NUMPAD_MINUS", "NUMPAD_PLUS", "NUMPAD_PERIOD",
        // Navigation
        "HOME", "END", "INSERT", "PAGEUP", "PAGEDOWN",
        // Arrow keys
        "UP", "DOWN", "LEFT", "RIGHT",
        // Misc
        "ACCENT", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "HYPHEN", "EQUAL"
    };
}

QStringList HotkeyManager::getAvailableModifiers()
{
    return QStringList{"CTRL", "SHIFT", "ALT", "META"};
}
