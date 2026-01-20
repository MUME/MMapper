// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "HotkeyManager.h"

#include "../configuration/configuration.h"
#include "../global/TextUtils.h"
#include "../global/logging.h"

#include <QDebug>
#include <QSettings>

HotkeyManager::HotkeyManager()
{
    setConfig().hotkeys.registerChangeCallback(m_configLifetime,
                                               [this]() { this->syncFromConfig(); });
    setConfig().hotkeys.registerResetCallback(m_configLifetime,
                                              [this]() { this->resetToDefaults(); });
    syncFromConfig();
    if (m_hotkeys.empty()) {
        resetToDefaults();
    }
}

void HotkeyManager::syncFromConfig()
{
    m_hotkeys.clear();
    const QVariantMap &data = getConfig().hotkeys.data();
    for (auto it = data.begin(); it != data.end(); ++it) {
        auto key = mmqt::toStdStringUtf8(it.key());
        auto value = mmqt::toStdStringUtf8(it.value().toString());
        Hotkey hk(key);
        if (hk.isValid()) {
            m_hotkeys[hk] = value;
        } else {
            MMLOG_WARNING() << "invalid hotkey" << key << value;
        }
    }
}

bool HotkeyManager::setHotkey(const Hotkey &hk, std::string command)
{
    if (!hk.isValid()) {
        return false;
    }

    QVariantMap data = getConfig().hotkeys.data();
    data[mmqt::toQStringUtf8(hk.to_string())] = mmqt::toQStringUtf8(command);
    setConfig().hotkeys.setData(std::move(data));
    return true;
}

bool HotkeyManager::removeHotkey(const Hotkey &hk)
{
    if (!hk.isRecognized()) {
        return false;
    }
    QVariantMap data = getConfig().hotkeys.data();
    const auto key = mmqt::toQStringUtf8(hk.to_string());
    if (!data.contains(key)) {
        return false;
    }
    data.remove(key);
    setConfig().hotkeys.setData(std::move(data));
    return true;
}

std::optional<std::string> HotkeyManager::getCommand(const Hotkey &hk) const
{
    if (!hk.isValid()) {
        return std::nullopt;
    }

    auto it = m_hotkeys.find(hk);
    if (it == m_hotkeys.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<std::pair<Hotkey, std::string>> HotkeyManager::getAllHotkeys() const
{
    std::vector<std::pair<Hotkey, std::string>> result;
    for (const auto &[hk, cmd] : m_hotkeys) {
        result.emplace_back(hk, cmd);
    }
    return result;
}

void HotkeyManager::resetToDefaults()
{
    QVariantMap data;
#define X_DEFAULT(key, cmd) data[key] = QString(cmd);
    XFOREACH_DEFAULT_HOTKEYS(X_DEFAULT)
#undef X_DEFAULT
    setConfig().hotkeys.setData(std::move(data));
}

void HotkeyManager::clear()
{
    setConfig().hotkeys.setData(QVariantMap());
}
