// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "GroupConfig.h"

#include <QSettings>

GroupConfig::GroupConfig(QString groupName)
    : m_groupName(std::move(groupName))
{}

void GroupConfig::read(const QSettings &settings)
{
    QVariantMap newData;
    for (const QString &key : settings.childKeys()) {
        newData[key] = settings.value(key);
    }
    setData(std::move(newData));
}

void GroupConfig::write(QSettings &settings) const
{
    settings.remove(""); // clear the current group
    for (auto it = m_data.begin(); it != m_data.end(); ++it) {
        settings.setValue(it.key(), it.value());
    }
}

void GroupConfig::setData(QVariantMap data)
{
    if (m_data == data) {
        return;
    }
    m_data = std::move(data);
    notifyChanged();
}

void GroupConfig::notifyChanged()
{
    m_changeMonitor.notifyAll();
}

void GroupConfig::registerChangeCallback(const ChangeMonitor::Lifetime &lifetime,
                                         ChangeMonitor::Function callback)
{
    m_changeMonitor.registerChangeCallback(lifetime, std::move(callback));
}

void GroupConfig::resetToDefault()
{
    m_resetMonitor.notifyAll();
}

void GroupConfig::registerResetCallback(const ChangeMonitor::Lifetime &lifetime,
                                        ChangeMonitor::Function callback)
{
    m_resetMonitor.registerChangeCallback(lifetime, std::move(callback));
}
