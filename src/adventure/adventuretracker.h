#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Mike Repass <mike.repass@gmail.com> (Taryn)

#include "adventure/adventuresession.h"
#include "adventure/lineparsers.h"
#include "observer/gameobserver.h"
#include <fstream>
#include <QObject>

class AdventureTracker final : public QObject
{
    Q_OBJECT
public:
    explicit AdventureTracker(GameObserver &observer, QObject *const parent = nullptr);

signals:
    void sig_accomplishedTask(double xpGained);
    void sig_achievedSomething(const QString &achievement, double xpGained);
    void sig_diedInGame(double xpLost);
    void sig_endedSession(const AdventureSession &session);
    void sig_gainedLevel();
    void sig_killedMob(const QString &mobName, double xpGained);
    void sig_receivedHint(const QString &hint);
    void sig_receivedNarrate(const QString &msg);
    void sig_receivedTell(const QString &msg);
    void sig_updatedSession(const AdventureSession &session);

public slots:
    void slot_onUserText(const QString &line);
    void slot_onUserGmcp(const GmcpMessage &msg);

private:
    void parseIfGoodbye(GmcpMessage msg);
    void parseIfReceivedComm(GmcpMessage msg);
    void parseIfUpdatedCharName(GmcpMessage msg);
    void parseIfUpdatedVitals(GmcpMessage msg);

    double checkpointXP();

    GameObserver &m_observer;

    std::unique_ptr<AdventureSession> m_session;

    AchievementParser m_achievementParser;
    DiedParser m_diedParser;
    GainedLevelParser m_gainedLevelParser;
    HintParser m_hintParser;
    KillAndXPParser m_killParser;
    AccomplishedTaskParser m_accomplishedTaskParser;
};
