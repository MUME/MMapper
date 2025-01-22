#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Mike Repass <mike.repass@gmail.com> (Taryn)

#include "../global/Signal2.h"
#include "../observer/gameobserver.h"
#include "adventuresession.h"
#include "lineparsers.h"

#include <memory>

#include <QObject>

class NODISCARD_QOBJECT AdventureTracker final : public QObject
{
    Q_OBJECT

private:
    GameObserver &m_observer;
    std::shared_ptr<AdventureSession> m_session;
    KillAndXPParser m_killParser;
    QString m_prevLine = "";
    Signal2Lifetime m_lifetime;

public:
    explicit AdventureTracker(GameObserver &observer, QObject *parent);

private:
    void parseIfGoodbye(const GmcpMessage &msg);
    void parseIfUpdatedCharName(const GmcpMessage &msg);
    void parseIfUpdatedVitals(const GmcpMessage &msg);
    NODISCARD double checkpointXP();

private:
    void onUserGmcp(const GmcpMessage &msg);
    void onUserText(const QString &line);

signals:
    void sig_accomplishedTask(double xpGained);
    void sig_achievedSomething(const QString &achievement, double xpGained);
    void sig_diedInGame(double xpLost);
    void sig_endedSession(const std::shared_ptr<AdventureSession> &session);
    void sig_gainedLevel();
    void sig_killedMob(const QString &mobName, double xpGained);
    void sig_receivedHint(const QString &hint);
    void sig_updatedSession(const std::shared_ptr<AdventureSession> &session);
};
