#pragma once

#include "observer/gameobserver.h"
#include <array>
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
    void sig_gainedLevel();
    void sig_killedMob(const QString &mobName, const double xpGained);
    void sig_receivedHint(const QString &hint);
    void sig_receivedNarrate(const QString &msg);
    void sig_receivedTell(const QString &msg);
    void sig_updatedXP(const double currentXP);

public slots:
    void slot_onUserText(const QByteArray &ba);
    void slot_onUserGmcp(const GmcpMessage &msg);

private:
    void parseIfAchievedSomething();
    void parseIfGainedALevel();
    void parseIfKillAndXP();
    void parseIfReceivedComm();
    void parseIfReceivedHint();
    void parseIfUpdatedChar();
    void parseIfUpdatedXP();

    void updateCharfromMud(QString charName);
    void updateXPfromMud(double currentXP);
    double checkpointXP();

    GameObserver &m_observer;

    // indexing is backwards, so [0] is most recent, [1] is prev, [2] is prev2, etc
    std::array<QString *, 5> m_lastLines = {nullptr, nullptr, nullptr, nullptr, nullptr};

    GmcpMessage *m_lastGmcpMessage = nullptr;
    QJsonDocument *m_lastGmcpJsonDoc = nullptr;

    std::optional<QString> m_currentCharName;

    std::optional<double> m_xpInitial, m_xpCheckpoint, m_xpCurrent;
};