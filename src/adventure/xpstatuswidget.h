#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Mike Repass <mike.repass@gmail.com> (Taryn)

#include "../global/macros.h"
#include "adventuretracker.h"

#include <memory>

#include <QLabel>
#include <QPushButton>
#include <QStatusBar>

class NODISCARD_QOBJECT XPStatusWidget : public QPushButton
{
    Q_OBJECT

private:
    QStatusBar *m_statusBar = nullptr;
    AdventureTracker &m_tracker;
    std::shared_ptr<AdventureSession> m_session;
    bool m_showPreference = true;

public:
    explicit XPStatusWidget(AdventureTracker &at, QStatusBar *sb, QWidget *parent);

public slots:
    void slot_configChanged(const std::type_info &configGroup);
    void slot_updatedSession(const std::shared_ptr<AdventureSession> &session);

protected:
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void readConfig();
    void updateContent();
};
