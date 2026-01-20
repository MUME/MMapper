#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors

#include "../global/ChangeMonitor.h"
#include "../global/RuleOf5.h"
#include "../global/macros.h"

#include <functional>

#include <QMap>
#include <QString>
#include <QVariant>

class QSettings;

class NODISCARD GroupConfig final
{
private:
    QString m_groupName;
    QVariantMap m_data;
    ChangeMonitor m_changeMonitor;
    ChangeMonitor m_resetMonitor;

public:
    GroupConfig() = delete;
    DELETE_CTORS_AND_ASSIGN_OPS(GroupConfig);

    explicit GroupConfig(QString groupName);

    void read(const QSettings &settings);
    void write(QSettings &settings) const;

    NODISCARD const QString &getName() const { return m_groupName; }
    NODISCARD const QVariantMap &data() const { return m_data; }
    void setData(QVariantMap data);

    void notifyChanged();
    void registerChangeCallback(const ChangeMonitor::Lifetime &lifetime,
                                ChangeMonitor::Function callback);

    void resetToDefault();
    void registerResetCallback(const ChangeMonitor::Lifetime &lifetime,
                               ChangeMonitor::Function callback);
};
