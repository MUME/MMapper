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
    void sig_receivedNarrate(const QString &msg);
    void sig_receivedTell(const QString &msg);
    void sig_killedMob(const QString &mobName);

public slots:
    void slot_onUserText(const QByteArray &ba);

private:
    GameObserver &m_observer;
};


