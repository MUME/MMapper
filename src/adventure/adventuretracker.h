#pragma once

#include "adventure/charstatus.h"
#include "adventure/lineparsers.h"
#include "observer/gameobserver.h"
#include <fstream>
#include <QObject>

class AdventureTracker final : public QObject
{
    Q_OBJECT
public:
    explicit AdventureTracker(GameObserver &observer, QObject *const parent = nullptr);
    ~AdventureTracker() final;

signals:
    void sig_achievedSomething(const QString &achievement, const double xpGained);
    void sig_died(const double xpLost);
    void sig_endedSession(const QString charName);
    void sig_gainedLevel();
    void sig_killedMob(const QString &mobName, const double xpGained);
    void sig_lostLevel(const double xpLost);
    void sig_receivedHint(const QString &hint);
    void sig_receivedNarrate(const QString &msg);
    void sig_receivedTell(const QString &msg);
    void sig_updatedXP(const double xpInitial, const double xpCurrent);
    void sig_updatedChar(const QString charName);

public slots:
    void slot_onUserText(const QByteArray &ba);
    void slot_onUserGmcp(const GmcpMessage &msg);

private:
    void parseIfReceivedComm(GmcpMessage msg, QJsonDocument doc);
    void parseIfUpdatedChar(GmcpMessage msg, QJsonDocument doc);
    void parseIfUpdatedXP(GmcpMessage msg, QJsonDocument doc);

    double checkpointXP();

    GameObserver &m_observer;

    std::optional<AdventureProgress> m_Progress;

    AchievementParser m_achievementParser;
    DiedParser m_diedParser;
    GainedLevelParser m_gainedLevelParser;
    HintParser m_hintParser;
    KillAndXPParser m_killParser;
    LostLevelParser m_lostLevelParser;
};
