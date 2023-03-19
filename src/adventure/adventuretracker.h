#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Mike Repass <mike.repass@gmail.com> (Taryn)

#include <QObject>

#include "adventure/adventuresession.h"
#include "adventure/lineparsers.h"
#include "observer/gameobserver.h"

class AdventureTracker final : public QObject
{
    Q_OBJECT
public:
    explicit AdventureTracker(GameObserver &observer, QObject *const parent = nullptr);

signals:
    void sig_accomplishedTask(double xpGained);
    void sig_achievedSomething(const QString &achievement, double xpGained);
    void sig_diedInGame(double xpLost);
    void sig_endedSession(const std::shared_ptr<AdventureSession> &session);
    void sig_gainedLevel();
    void sig_killedMob(const QString &mobName, double xpGained);
    void sig_receivedHint(const QString &hint);
    void sig_updatedSession(const std::shared_ptr<AdventureSession> &session);

public slots:
    void slot_onUserText(const QString &line);
    void slot_onUserGmcp(const GmcpMessage &msg);

private:
    void parseIfGoodbye(const GmcpMessage &msg);
    void parseIfUpdatedCharName(const GmcpMessage &msg);
    void parseIfUpdatedVitals(const GmcpMessage &msg);

    double checkpointXP();

    GameObserver &m_observer;

    std::shared_ptr<AdventureSession> m_session;

    AchievementParser m_achievementParser;
    DiedParser m_diedParser;
    GainedLevelParser m_gainedLevelParser;
    HintParser m_hintParser;
    KillAndXPParser m_killParser;
    AccomplishedTaskParser m_accomplishedTaskParser;
};
