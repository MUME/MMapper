/*
* CTimers.cpp
*
*  Created on: Mar 13, 2011
*      Author: aza
*/

#include "CTimers.h"
#include <QMutexLocker>
#include <QSettings>
#include <QTimer>

CTimers::CTimers(QObject *parent)
    : QObject(parent)
{
    m_nextId = 1;
}

CTimers::~CTimers() {}

void CTimers::addTimer(QByteArray name, QByteArray desc)
{
    QMutexLocker locker(&m_lock);

    TTimer timer;

    timer.name = name;
    timer.desc = desc;
    timer.timer.start();
    timer.id = m_nextId++;

    m_timers.push_back(timer);
}

bool CTimers::removeCountdown(QByteArray name)
{
    QMutexLocker locker(&m_lock);

    for (std::list<TTimer>::iterator it = m_countdowns.begin(); it != m_countdowns.end(); ++it) {
        if (it->name == name) {
            m_countdowns.erase(it);
            return true;
        }
    }

    return false;
}

bool CTimers::removeTimer(QByteArray name)
{
    QMutexLocker locker(&m_lock);

    for (std::list<TTimer>::iterator it = m_timers.begin(); it != m_timers.end(); ++it) {
        if (it->name == name) {
            m_timers.erase(it);
            return true;
        }
    }

    return false;
}

void CTimers::addCountdown(QByteArray name, QByteArray desc, int time)
{
    QMutexLocker locker(&m_lock);

    TTimer countdown;
    countdown.name = name;
    countdown.desc = desc;
    countdown.timer.start();
    countdown.id = m_nextId++;
    countdown.duration = time;

    // append the shot-down event pending
    QTimer::singleShot(time, this, SLOT(finishCountdownTimer()));

    m_countdowns.push_back(countdown);
}

void CTimers::finishCountdownTimer()
{
    QMutexLocker locker(&m_lock);

    int deleted = 0;
    int delta = 1;

    std::list<TTimer>::iterator it = m_countdowns.begin();
    while(it != m_countdowns.end()) {
        int diff = static_cast<int>(it->duration - it->timer.elapsed());
        if (diff <= 0) {
            emit sig_sendTimersUpdateToUser(
                QString("--[ Countdown timer %1 < %2 > finished.\r\n").arg(it->name, it->desc));
            it = m_countdowns.erase(it);
            deleted++;
            if (diff < delta) {
                // keep the smallest diff, to fire the event again
                delta = diff;
            }
        } else {
            ++it;
        }
    }

    // but still, at least 1ms
    delta = std::max(delta, 1);

    // as of Qt 5.2.1 singleShot timers are not precise enough and might, easily, fire earlier!
    // Aza: check? I have no idea what this is about anymore
    if (deleted == 0) {
        QTimer::singleShot(delta, this, SLOT(finishCountdownTimer()));
    }
}

QByteArray CTimers::getTimers()
{
    if (m_timers.size() == 0)
        return "";

    QString line = "Timers:\r\n";
    for (std::list<TTimer>::iterator it = m_timers.begin(); it != m_timers.end(); ++it) {
        line += QString("- %1 < %2 > (up for - %3)\r\n")
                    .arg(it->name.data(), it->desc.data(), msToMinSec(it->timer.elapsed()));
    }
    return line.toLocal8Bit();
}

QByteArray CTimers::getCountdowns()
{
    if (m_countdowns.size() == 0)
        return "";

    QString line = "Countdowns:\r\n";
    for (std::list<TTimer>::iterator it = m_countdowns.begin(); it != m_countdowns.end(); ++it) {
        line += QString("- %1 < %2 > (up for - %3, left - %4)\r\n")
                    .arg(it->name.data(), it->desc.data(),
                         msToMinSec(it->timer.elapsed()),
                         msToMinSec(it->duration - it->timer.elapsed())
                     );
    }
    return line.toLocal8Bit();
}

QByteArray CTimers::getStatCommandEntry()
{
    QMutexLocker locker(&m_lock);
    return getCountdowns() + getTimers();
}

void CTimers::clear()
{
    QMutexLocker locker(&m_lock);

    m_countdowns.clear();
    m_timers.clear();
}
