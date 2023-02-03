#pragma once

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
    ~AdventureTracker() override;

signals:
    void sig_accomplishedTask(double xpGained);
    void sig_achievedSomething(const QString &achievement, double xpGained);
    void sig_died(double xpLost);
    void sig_endedSession(AdventureSession session);
    void sig_gainedLevel();
    void sig_killedMob(const QString &mobName, double xpGained);
    void sig_receivedHint(const QString &hint);
    void sig_receivedNarrate(const QString &msg);
    void sig_receivedTell(const QString &msg);
    void sig_updatedSession(const AdventureSession &session);

public slots:
    void slot_onUserText(const QByteArray &ba);
    void slot_onUserGmcp(const GmcpMessage &msg);

private:
    void parseIfGoodbye(GmcpMessage msg);
    void parseIfReceivedComm(GmcpMessage msg);
    void parseIfUpdatedChar(GmcpMessage msg);
    void parseIfUpdatedXP(GmcpMessage msg);

    double checkpointXP();

    GameObserver &m_observer;

    std::optional<AdventureSession> m_Session;

    AchievementParser m_achievementParser;
    DiedParser m_diedParser;
    GainedLevelParser m_gainedLevelParser;
    HintParser m_hintParser;
    KillAndXPParser m_killParser;
    AccomplishedTaskParser m_accomplishedTaskParser;
};
