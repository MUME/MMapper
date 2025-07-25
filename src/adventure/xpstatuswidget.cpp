// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Mike Repass <mike.repass@gmail.com> (Taryn)

#include "xpstatuswidget.h"

#include "../configuration/configuration.h"
#include "adventuresession.h"
#include "adventuretracker.h"

#include <memory>

XPStatusWidget::XPStatusWidget(AdventureTracker &at, QStatusBar *const sb, QWidget *const parent)
    : QPushButton(parent)
    , m_statusBar{sb}
    , m_tracker{at}
{
    setFlat(true);
    setStyleSheet("QPushButton { border: none; outline: none; }");
    setMouseTracking(true);

    updateContent();

    connect(&m_tracker,
            &AdventureTracker::sig_updatedSession,
            this,
            &XPStatusWidget::slot_updatedSession);

    connect(&m_tracker,
            &AdventureTracker::sig_endedSession,
            this,
            &XPStatusWidget::slot_updatedSession);

    setConfig().adventurePanel.registerChangeCallback(m_lifetime, [this]() { updateContent(); });
}

void XPStatusWidget::updateContent()
{
    if (getConfig().adventurePanel.getDisplayXPStatus() && m_session != nullptr) {
        auto xpSession = m_session->xp().gainedSession();
        auto tpSession = m_session->tp().gainedSession();
        auto xpf = AdventureSession::formatPoints(xpSession);
        auto tpf = AdventureSession::formatPoints(tpSession);
        auto msg = QString("%1 Session: %2 XP %3 TP").arg(m_session->name(), xpf, tpf);
        setText(msg);
        show();
        repaint();
    } else {
        setText("");
        hide();
    }
}

void XPStatusWidget::slot_updatedSession(const std::shared_ptr<AdventureSession> &session)
{
    m_session = session;
    updateContent();
}

void XPStatusWidget::enterEvent(QEnterEvent *event)
{
    if (m_session != nullptr) {
        auto xpHourly = m_session->calculateHourlyRateXP();
        auto tpHourly = m_session->calculateHourlyRateTP();
        auto xpf = AdventureSession::formatPoints(xpHourly);
        auto tpf = AdventureSession::formatPoints(tpHourly);
        auto msg = QString("Hourly rate: %1 XP %2 TP").arg(xpf, tpf);
        m_statusBar->showMessage(msg);
    }

    QWidget::enterEvent(event);
}

void XPStatusWidget::leaveEvent(QEvent *event)
{
    m_statusBar->clearMessage();

    QWidget::leaveEvent(event);
}
