#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "../configuration/NamedConfig.h"

#include <QWidget>

class QCheckBox;
class QPushButton;

class NODISCARD_QOBJECT VisibilityFilterWidget final : public QWidget
{
    Q_OBJECT

signals:
    void sig_visibilityChanged();
    void sig_connectionsVisibilityChanged();

private:
    QCheckBox *m_genericCheckBox = nullptr;
    QCheckBox *m_herbCheckBox = nullptr;
    QCheckBox *m_riverCheckBox = nullptr;
    QCheckBox *m_placeCheckBox = nullptr;
    QCheckBox *m_mobCheckBox = nullptr;
    QCheckBox *m_commentCheckBox = nullptr;
    QCheckBox *m_roadCheckBox = nullptr;
    QCheckBox *m_objectCheckBox = nullptr;
    QCheckBox *m_actionCheckBox = nullptr;
    QCheckBox *m_localityCheckBox = nullptr;
    QCheckBox *m_connectionsCheckBox = nullptr;

    QPushButton *m_showAllButton = nullptr;
    QPushButton *m_hideAllButton = nullptr;

    ChangeMonitor::Lifetime m_changeMonitorLifetime;

public:
    explicit VisibilityFilterWidget(QWidget *parent = nullptr);
    ~VisibilityFilterWidget() final = default;

private:
    void setupUI();
    void connectSignals();
    void setupChangeCallbacks();
    void updateCheckboxStates();
};
