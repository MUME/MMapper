/*
* CTimers.cpp
*
*  Created on: Mar 13, 2011
*      Author: aza
*/

#include "CTimers.h"
#include <QMutexLocker>
#include <QTimer>

CTimers::CTimers(QObject *parent)
    : QObject(parent)
{
    m_nextId = 1;
}

CTimers::~CTimers()
{
    // TODO Auto-generated destructor stub
}

void CTimers::addTimer(QByteArray name, QByteArray desc)
{
    QMutexLocker locker(&m_lock);

    TTimer *l_timer = new TTimer;
    l_timer->name = name;
    l_timer->desc = desc;
    l_timer->timer.start();
    l_timer->id = m_nextId++;

    m_timers.append(l_timer);
}

bool CTimers::removeCountdown(QByteArray name)
{
    QMutexLocker locker(&m_lock);

    for (int i = 0; i < m_countdowns.size(); i++) {
        TTimer *s = m_countdowns[i];
        if (s->name == name) {
            m_countdowns.removeAt(i);
            delete s;
            return true;
        }
    }
    return false;
}

bool CTimers::removeTimer(QByteArray name)
{
    QMutexLocker locker(&m_lock);

    for (int i = 0; i < m_timers.size(); i++) {
        TTimer *s = m_timers[i];
        if (s->name == name) {
            m_timers.removeAt(i);
            delete s;
            return true;
        }
    }
    return false;
}

void CTimers::addCountdown(QByteArray name, QByteArray desc, int time)
{
    QMutexLocker locker(&m_lock);

    TTimer *l_timer = new TTimer;
    l_timer->name = name;
    l_timer->desc = desc;
    l_timer->timer.start();
    l_timer->id = m_nextId++;
    l_timer->duration = time;

    // append the shot-down event pending

    QTimer::singleShot(time, this, SLOT(finishCountdownTimer()));

    m_countdowns.append(l_timer);
}

void CTimers::finishCountdownTimer()
{
    QMutexLocker locker(&m_lock);

    int deleted = 0;
    int delta = 1;
    for (int i = 0; i < m_countdowns.size(); i++) {
        TTimer *s = m_countdowns[i];

        //send_to_user("--[ testing countdown timer:  %s < %s > dur: %i elapsed: %i finished.\r\n\r\n", (const char *) s->name, (const char *) s->desc, s->duration, s->timer.elapsed());
        int diff = s->duration - s->timer.elapsed();

        if (diff <= 0) {
            // Fixme: restore this functionality
            // send_to_user("--[ Countdown timer %s < %s > finished.\r\n\r\n", (const char *) s->name, (const char *) s->desc);
            // send_prompt();
            m_countdowns.removeAt(i);
            delete s;
            deleted++;
        } else if (diff < delta) {
            delta = diff;
        }
    }

    // as of Qt 5.2.1 (at previous) singleShot timers are not precise enough
    // and might, easilly, fire earlier!
    if (deleted == 0) {
        // in this case, refire the event in at least 1ms
        QTimer::singleShot(delta, this, SLOT(finishCountdownTimer()));
    }
}

QByteArray CTimers::checkTimersLine()
{
    QString s = getStatCommandEntry();

    //    if (s != "")
    //    	s+= "Normal spells:\r\n";
    return qPrintable(s);
}

QByteArray CTimers::getTimers()
{
    if (m_timers.size() == 0)
        return "";

    QString line = "Timers:\r\n";
    for (int i = 0; i < m_timers.size(); i++) {
        TTimer *s = m_timers[i];
        line += QString("- %1 < %2 > (up for - %3)\r\n")
                    .arg((const char *) s->name)
                    .arg((const char *) s->desc)
                    .arg(msToMinSec(s->timer.elapsed()));
    }
    return line.toLocal8Bit();
}

QByteArray CTimers::getCountdowns()
{
    if (m_countdowns.size() == 0)
        return "";

    QString line = "Countdowns:\r\n";
    for (int i = 0; i < m_countdowns.size(); i++) {
        TTimer *s = m_countdowns[i];
        line += QString("- %1 < %2 > (up for - %3, left - %4)\r\n")
                    .arg((const char *) s->name)
                    .arg((const char *) s->desc)
                    .arg(msToMinSec(s->timer.elapsed()))
                    .arg(msToMinSec(s->duration - s->timer.elapsed()));
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

    for (int i = 0; i < m_countdowns.size(); i++) {
        TTimer *s = m_countdowns[m_countdowns.size() - 1];
        m_countdowns.removeLast();
        delete s;
    }

    m_countdowns.clear();
    for (int i = 0; i < m_timers.size(); i++) {
        TTimer *s = m_timers[m_timers.size() - 1];
        m_timers.removeLast();
        delete s;
    }
}
