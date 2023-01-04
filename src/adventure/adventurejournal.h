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
    void sig_receivedComm(const QString &comm);

public slots:
    void slot_updateJournal(const QByteArray &ba);

private:
    GameObserver &m_observer;
    std::fstream m_debugFile;
    QList<QString> m_commsList;
};


