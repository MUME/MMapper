// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Mike Repass <mike.repass@gmail.com> (Taryn)

#include "xpstatuswidget.h"
#include "../configuration/configuration.h"
#include "adventuresession.h"
#include "adventuretracker.h"
#include "adventurewidget.h"

XPStatusWidget::XPStatusWidget(AdventureTracker &at, QStatusBar *sb, QWidget *parent)
    : QPushButton(parent)
    , m_statusBar{sb}
    , m_tracker{at}
    , m_session{}
{
    setFlat(true);
    setStyleSheet("QPushButton { border: none; outline: none; }");
    setMouseTracking(true);

    readConfig();
    updateContent();

    connect(&m_tracker,
            &AdventureTracker::sig_updatedSession,
            this,
            &XPStatusWidget::slot_updatedSession);

    connect(&m_tracker,
            &AdventureTracker::sig_endedSession,
            this,
            &XPStatusWidget::slot_updatedSession);
}

void XPStatusWidget::readConfig()
{
    m_showPreference = getConfig().adventurePanel.getDisplayXPStatus();
}

void XPStatusWidget::updateContent()
{
    if (m_showPreference && m_session.has_value()) {
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

void XPStatusWidget::slot_configChanged(const std::type_info &configGroup)
{
    if (configGroup != typeid(Configuration::AdventurePanelSettings))
        return;

    readConfig();
    updateContent();
}

void XPStatusWidget::slot_updatedSession(const AdventureSession &session)
{
    m_session = session;
    updateContent();
}

void XPStatusWidget::enterEvent(QEvent *event)
{
    if (m_session.has_value()) {
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
