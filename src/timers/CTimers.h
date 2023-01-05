#pragma once
//
// Created by Azazello on 16.12.22.
//

#include <QByteArray>
#include <QElapsedTimer>
#include <QMutex>
#include <QObject>

struct TTimer
{
    int id;
    QByteArray name;
    QByteArray desc;
    int duration;
    QElapsedTimer timer;
};

class CTimers : public QObject
{
    Q_OBJECT

    QMutex m_lock;

    int m_nextId;
    std::list<TTimer> m_timers;
    std::list<TTimer> m_countdowns;

    QByteArray getTimers();
    QByteArray getCountdowns();
signals:
    void sig_sendTimersUpdateToUser(const QString str);

public:
    explicit CTimers(QObject *parent);
    virtual ~CTimers();

    static QString msToMinSec(qint64 ms)
    {
        QString s;
        int min;
        qint64 sec;

        sec = ms / 1000;
        min = static_cast<int>(sec / 60);
        sec = sec % 60;

        s = QString("%1%2:%3%4").arg(min / 10).arg(min % 10).arg(sec / 10).arg(sec % 10);

        return s;
    }

    void addTimer(QByteArray name, QByteArray desc);
    void addCountdown(QByteArray name, QByteArray desc, int time);

    bool removeTimer(QByteArray name);
    bool removeCountdown(QByteArray name);

    // for stat command representation
    QByteArray getStatCommandEntry();

    void clear();

public slots:
    void finishCountdownTimer();
};
