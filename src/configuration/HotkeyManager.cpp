// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "HotkeyManager.h"

#include "../global/TextUtils.h"

#include <unordered_map>
#include <unordered_set>

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

// Key name to Qt::Key mapping
const QHash<QString, int> &getKeyNameToQtKeyMap()
{
    static const QHash<QString, int> map{// Function keys
                                         {"F1", Qt::Key_F1},
                                         {"F2", Qt::Key_F2},
                                         {"F3", Qt::Key_F3},
                                         {"F4", Qt::Key_F4},
                                         {"F5", Qt::Key_F5},
                                         {"F6", Qt::Key_F6},
                                         {"F7", Qt::Key_F7},
                                         {"F8", Qt::Key_F8},
                                         {"F9", Qt::Key_F9},
                                         {"F10", Qt::Key_F10},
                                         {"F11", Qt::Key_F11},
                                         {"F12", Qt::Key_F12},
                                         // Numpad
                                         {"NUMPAD0", Qt::Key_0},
                                         {"NUMPAD1", Qt::Key_1},
                                         {"NUMPAD2", Qt::Key_2},
                                         {"NUMPAD3", Qt::Key_3},
                                         {"NUMPAD4", Qt::Key_4},
                                         {"NUMPAD5", Qt::Key_5},
                                         {"NUMPAD6", Qt::Key_6},
                                         {"NUMPAD7", Qt::Key_7},
                                         {"NUMPAD8", Qt::Key_8},
                                         {"NUMPAD9", Qt::Key_9},
                                         {"NUMPAD_SLASH", Qt::Key_Slash},
                                         {"NUMPAD_ASTERISK", Qt::Key_Asterisk},
                                         {"NUMPAD_MINUS", Qt::Key_Minus},
                                         {"NUMPAD_PLUS", Qt::Key_Plus},
                                         {"NUMPAD_PERIOD", Qt::Key_Period},
                                         // Navigation
                                         {"HOME", Qt::Key_Home},
                                         {"END", Qt::Key_End},
                                         {"INSERT", Qt::Key_Insert},
                                         {"PAGEUP", Qt::Key_PageUp},
                                         {"PAGEDOWN", Qt::Key_PageDown},
                                         // Arrow keys
                                         {"UP", Qt::Key_Up},
                                         {"DOWN", Qt::Key_Down},
                                         {"LEFT", Qt::Key_Left},
                                         {"RIGHT", Qt::Key_Right},
                                         // Misc
                                         {"ACCENT", Qt::Key_QuoteLeft},
                                         {"0", Qt::Key_0},
                                         {"1", Qt::Key_1},
                                         {"2", Qt::Key_2},
                                         {"3", Qt::Key_3},
                                         {"4", Qt::Key_4},
                                         {"5", Qt::Key_5},
                                         {"6", Qt::Key_6},
                                         {"7", Qt::Key_7},
                                         {"8", Qt::Key_8},
                                         {"9", Qt::Key_9},
                                         {"HYPHEN", Qt::Key_Minus},
                                         {"EQUAL", Qt::Key_Equal}};
    return map;
}

// Qt::Key to key name mapping (for non-numpad keys)
const QHash<int, QString> &getQtKeyToKeyNameMap()
{
    static const QHash<int, QString> map{// Function keys
                                         {Qt::Key_F1, "F1"},
                                         {Qt::Key_F2, "F2"},
                                         {Qt::Key_F3, "F3"},
                                         {Qt::Key_F4, "F4"},
                                         {Qt::Key_F5, "F5"},
                                         {Qt::Key_F6, "F6"},
                                         {Qt::Key_F7, "F7"},
                                         {Qt::Key_F8, "F8"},
                                         {Qt::Key_F9, "F9"},
                                         {Qt::Key_F10, "F10"},
                                         {Qt::Key_F11, "F11"},
                                         {Qt::Key_F12, "F12"},
                                         // Navigation
                                         {Qt::Key_Home, "HOME"},
                                         {Qt::Key_End, "END"},
                                         {Qt::Key_Insert, "INSERT"},
                                         {Qt::Key_PageUp, "PAGEUP"},
                                         {Qt::Key_PageDown, "PAGEDOWN"},
                                         // Arrow keys
                                         {Qt::Key_Up, "UP"},
                                         {Qt::Key_Down, "DOWN"},
                                         {Qt::Key_Left, "LEFT"},
                                         {Qt::Key_Right, "RIGHT"},
                                         // Misc
                                         {Qt::Key_QuoteLeft, "ACCENT"},
                                         {Qt::Key_Equal, "EQUAL"}};
    return map;
}

// Numpad Qt::Key to key name mapping (requires KeypadModifier to be set)
const QHash<int, QString> &getNumpadQtKeyToKeyNameMap()
{
    static const QHash<int, QString> map{{Qt::Key_0, "NUMPAD0"},
                                         {Qt::Key_1, "NUMPAD1"},
                                         {Qt::Key_2, "NUMPAD2"},
                                         {Qt::Key_3, "NUMPAD3"},
                                         {Qt::Key_4, "NUMPAD4"},
                                         {Qt::Key_5, "NUMPAD5"},
                                         {Qt::Key_6, "NUMPAD6"},
                                         {Qt::Key_7, "NUMPAD7"},
                                         {Qt::Key_8, "NUMPAD8"},
                                         {Qt::Key_9, "NUMPAD9"},
                                         {Qt::Key_Slash, "NUMPAD_SLASH"},
                                         {Qt::Key_Asterisk, "NUMPAD_ASTERISK"},
                                         {Qt::Key_Minus, "NUMPAD_MINUS"},
                                         {Qt::Key_Plus, "NUMPAD_PLUS"},
                                         {Qt::Key_Period, "NUMPAD_PERIOD"}};
    return map;
}

// Non-numpad digit/symbol key names
const QHash<int, QString> &getNonNumpadDigitKeyNameMap()
{
    static const QHash<int, QString> map{{Qt::Key_0, "0"},
                                         {Qt::Key_1, "1"},
                                         {Qt::Key_2, "2"},
                                         {Qt::Key_3, "3"},
                                         {Qt::Key_4, "4"},
                                         {Qt::Key_5, "5"},
                                         {Qt::Key_6, "6"},
                                         {Qt::Key_7, "7"},
                                         {Qt::Key_8, "8"},
                                         {Qt::Key_9, "9"},
                                         {Qt::Key_Minus, "HYPHEN"}};
    return map;
}

// Static set of valid base key names for validation
// Derived from HotkeyManager::getAvailableKeyNames() to avoid duplication and drift
const QSet<QString> &getValidBaseKeys()
{
    static const QSet<QString> validKeys = []() {
        QSet<QString> keys;
        for (const QString &key : HotkeyManager::getAvailableKeyNames()) {
            keys.insert(key);
        }
        return keys;
    }();
    return validKeys;
}

// Check if key name is a numpad key
bool isNumpadKeyName(const QString &keyName)
{
    return keyName.startsWith("NUMPAD");
}

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
            QString keyStr = normalizeKeyString(match.captured(1));
            QString commandQStr = match.captured(2).trimmed();
            if (!keyStr.isEmpty() && !commandQStr.isEmpty()) {
                // Convert string to HotkeyKey for fast lookup
                HotkeyKey hk = stringToHotkeyKey(keyStr);
                if (hk.key != 0) {
                    // Convert command to std::string for storage (cold path - OK)
                    std::string command = mmqt::toStdStringUtf8(commandQStr);
                    m_hotkeys[hk] = command;
                    m_orderedHotkeys.emplace_back(keyStr, command);
                }
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

bool HotkeyManager::setHotkey(const QString &keyName, const QString &command)
{
    QString normalizedKey = normalizeKeyString(keyName);
    if (normalizedKey.isEmpty()) {
        return false; // Invalid key name
    }

    // Update or add in raw content
    static const QRegularExpression hotkeyLineRegex(R"(^(\s*_hotkey\s+)(\S+)(\s+)(.+)$)",
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
    return true;
}

void HotkeyManager::removeHotkey(const QString &keyName)
{
    QString normalizedKey = normalizeKeyString(keyName);
    if (normalizedKey.isEmpty()) {
        return;
    }

    HotkeyKey hk = stringToHotkeyKey(normalizedKey);
    if (m_hotkeys.count(hk) == 0) {
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

std::string HotkeyManager::getCommand(int key, Qt::KeyboardModifiers modifiers, bool isNumpad) const
{
    // Strip KeypadModifier from modifiers - numpad distinction is tracked via isNumpad flag
    HotkeyKey hk(key, modifiers & ~Qt::KeypadModifier, isNumpad);
    auto it = m_hotkeys.find(hk);
    if (it != m_hotkeys.end()) {
        return it->second;
    }
    return std::string();
}

std::string HotkeyManager::getCommand(const QString &keyName) const
{
    QString normalizedKey = normalizeKeyString(keyName);
    if (normalizedKey.isEmpty()) {
        return std::string();
    }

    HotkeyKey hk = stringToHotkeyKey(normalizedKey);
    if (hk.key == 0) {
        return std::string();
    }

    auto it = m_hotkeys.find(hk);
    if (it != m_hotkeys.end()) {
        return it->second;
    }
    return std::string();
}

QString HotkeyManager::getCommandQString(int key,
                                         Qt::KeyboardModifiers modifiers,
                                         bool isNumpad) const
{
    const std::string cmd = getCommand(key, modifiers, isNumpad);
    if (cmd.empty()) {
        return QString();
    }
    return mmqt::toQStringUtf8(cmd);
}

QString HotkeyManager::getCommandQString(const QString &keyName) const
{
    const std::string cmd = getCommand(keyName);
    if (cmd.empty()) {
        return QString();
    }
    return mmqt::toQStringUtf8(cmd);
}

bool HotkeyManager::hasHotkey(const QString &keyName) const
{
    QString normalizedKey = normalizeKeyString(keyName);
    if (normalizedKey.isEmpty()) {
        return false;
    }

    HotkeyKey hk = stringToHotkeyKey(normalizedKey);
    return hk.key != 0 && m_hotkeys.count(hk) > 0;
}

QString HotkeyManager::normalizeKeyString(const QString &keyString)
{
    // Split by '+' to get individual parts
    QStringList parts = keyString.split('+', Qt::SkipEmptyParts);

    if (parts.isEmpty()) {
        qWarning() << "HotkeyManager: empty or invalid key string:" << keyString;
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
        } else {
            qWarning() << "HotkeyManager: unrecognized modifier:" << part << "in:" << keyString;
        }
    }

    // Validate the base key
    QString upperBaseKey = baseKey.toUpper();
    if (!isValidBaseKey(upperBaseKey)) {
        qWarning() << "HotkeyManager: invalid base key:" << baseKey << "in:" << keyString;
        return QString();
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
    normalizedParts << upperBaseKey;

    return normalizedParts.join("+");
}

HotkeyKey HotkeyManager::stringToHotkeyKey(const QString &keyString)
{
    QString normalized = normalizeKeyString(keyString);
    if (normalized.isEmpty()) {
        return HotkeyKey();
    }

    QStringList parts = normalized.split('+', Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        return HotkeyKey();
    }

    QString baseKey = parts.last();
    parts.removeLast();

    // Build modifiers
    Qt::KeyboardModifiers mods = Qt::NoModifier;
    for (const QString &part : parts) {
        if (part == "CTRL") {
            mods |= Qt::ControlModifier;
        } else if (part == "SHIFT") {
            mods |= Qt::ShiftModifier;
        } else if (part == "ALT") {
            mods |= Qt::AltModifier;
        } else if (part == "META") {
            mods |= Qt::MetaModifier;
        }
    }

    // Check if this is a numpad key
    bool isNumpad = isNumpadKeyName(baseKey);

    // Convert base key name to Qt::Key
    int qtKey = baseKeyNameToQtKey(baseKey);
    if (qtKey == 0) {
        return HotkeyKey();
    }

    return HotkeyKey(qtKey, mods, isNumpad);
}

QString HotkeyManager::hotkeyKeyToString(const HotkeyKey &hk)
{
    if (hk.key == 0) {
        return QString();
    }

    QStringList parts;

    // Add modifiers in canonical order
    if (hk.modifiers & Qt::ControlModifier) {
        parts << "CTRL";
    }
    if (hk.modifiers & Qt::ShiftModifier) {
        parts << "SHIFT";
    }
    if (hk.modifiers & Qt::AltModifier) {
        parts << "ALT";
    }
    if (hk.modifiers & Qt::MetaModifier) {
        parts << "META";
    }

    // Add the base key name - use numpad map if isNumpad is set
    QString keyName;
    if (hk.isNumpad) {
        keyName = getNumpadQtKeyToKeyNameMap().value(hk.key);
    }
    if (keyName.isEmpty()) {
        keyName = qtKeyToBaseKeyName(hk.key);
    }
    if (keyName.isEmpty()) {
        return QString();
    }
    parts << keyName;

    return parts.join("+");
}

int HotkeyManager::baseKeyNameToQtKey(const QString &keyName)
{
    auto it = getKeyNameToQtKeyMap().find(keyName.toUpper());
    if (it != getKeyNameToQtKeyMap().end()) {
        return it.value();
    }
    return 0;
}

QString HotkeyManager::qtKeyToBaseKeyName(int qtKey)
{
    // First check regular keys
    auto it = getQtKeyToKeyNameMap().find(qtKey);
    if (it != getQtKeyToKeyNameMap().end()) {
        return it.value();
    }

    // Check non-numpad digit keys
    auto it2 = getNonNumpadDigitKeyNameMap().find(qtKey);
    if (it2 != getNonNumpadDigitKeyNameMap().end()) {
        return it2.value();
    }

    return QString();
}

void HotkeyManager::resetToDefaults()
{
    m_rawContent = DEFAULT_HOTKEYS_CONTENT;
    parseRawContent();
    saveToSettings();
}

void HotkeyManager::clear()
{
    m_hotkeys.clear();
    m_orderedHotkeys.clear();
    m_rawContent.clear();
}

std::vector<QString> HotkeyManager::getAllKeyNames() const
{
    std::vector<QString> result;
    result.reserve(m_orderedHotkeys.size());
    for (const auto &pair : m_orderedHotkeys) {
        result.push_back(pair.first);
    }
    return result;
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

bool HotkeyManager::isValidBaseKey(const QString &baseKey)
{
    return getValidBaseKeys().contains(baseKey.toUpper());
}

std::vector<QString> HotkeyManager::getAvailableKeyNames()
{
    return std::vector<QString>{// Function keys
                                "F1",
                                "F2",
                                "F3",
                                "F4",
                                "F5",
                                "F6",
                                "F7",
                                "F8",
                                "F9",
                                "F10",
                                "F11",
                                "F12",
                                // Numpad
                                "NUMPAD0",
                                "NUMPAD1",
                                "NUMPAD2",
                                "NUMPAD3",
                                "NUMPAD4",
                                "NUMPAD5",
                                "NUMPAD6",
                                "NUMPAD7",
                                "NUMPAD8",
                                "NUMPAD9",
                                "NUMPAD_SLASH",
                                "NUMPAD_ASTERISK",
                                "NUMPAD_MINUS",
                                "NUMPAD_PLUS",
                                "NUMPAD_PERIOD",
                                // Navigation
                                "HOME",
                                "END",
                                "INSERT",
                                "PAGEUP",
                                "PAGEDOWN",
                                // Arrow keys
                                "UP",
                                "DOWN",
                                "LEFT",
                                "RIGHT",
                                // Misc
                                "ACCENT",
                                "0",
                                "1",
                                "2",
                                "3",
                                "4",
                                "5",
                                "6",
                                "7",
                                "8",
                                "9",
                                "HYPHEN",
                                "EQUAL"};
}

std::vector<QString> HotkeyManager::getAvailableModifiers()
{
    return std::vector<QString>{"CTRL", "SHIFT", "ALT", "META"};
}
