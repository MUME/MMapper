#pragma once

#include "observer/gameobserver.h"
#include <fstream>
#include <QObject>

class AdventureJournal final : public QObject
{
    Q_OBJECT
public:
    explicit AdventureJournal(GameObserver &observer, QObject *const parent = nullptr);
    ~AdventureJournal() final;

signals:
    void sig_killedMob(const QString &mobName);
    void sig_receivedNarrate(const QString &msg);
    void sig_receivedTell(const QString &msg);
    void sig_updatedXP(const double currentXP);

public slots:
    void slot_onUserText(const QByteArray &ba);
    void slot_onUserGmcp(const GmcpMessage &gmcpMessage);

private:
    GameObserver &m_observer;
};
