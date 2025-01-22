#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Mike Repass <mike.repass@gmail.com> (Taryn)

#include "../global/ChangeMonitor.h"
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
    Signal2Lifetime m_lifetime;

public:
    explicit XPStatusWidget(AdventureTracker &at, QStatusBar *sb, QWidget *parent);

protected:
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void updateContent();

public slots:
    void slot_updatedSession(const std::shared_ptr<AdventureSession> &session);
};
