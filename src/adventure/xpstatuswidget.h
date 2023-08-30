#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Mike Repass <mike.repass@gmail.com> (Taryn)

#include <QLabel>
#include <QPushButton>
#include <QStatusBar>

#include "adventure/adventuretracker.h"

class XPStatusWidget : public QPushButton
{
    Q_OBJECT
public:
    explicit XPStatusWidget(AdventureTracker &at,
                            QStatusBar *sb = nullptr,
                            QWidget *parent = nullptr);

public slots:
    void slot_configChanged(const std::type_info &configGroup);
    void slot_updatedSession(const std::shared_ptr<AdventureSession> &session);

protected:
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    void enterEvent(QEvent *event) override;
#else
    void enterEvent(QEnterEvent *event) override;
#endif
    void leaveEvent(QEvent *event) override;

private:
    void readConfig();
    void updateContent();

    bool m_showPreference = true;

    QStatusBar *m_statusBar;
    AdventureTracker &m_tracker;
    std::shared_ptr<AdventureSession> m_session;
};
