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

    QSettings conf("spells.ini", QSettings::IniFormat);

    auto size = conf.beginReadArray("Spells");
    for (int i = 0; i < size; ++i) {
        conf.setArrayIndex(i);
        TSpell spell;

        spell.up = false;
        spell.silently_up = false;
        spell.addon = conf.value("addon", 0).toBool();
        spell.name = conf.value("name").toByteArray();
        spell.up_mes = conf.value("upMessage").toByteArray();
        spell.refresh_mes = conf.value("refreshMessage").toByteArray();
        spell.down_mes = conf.value("downMessage").toByteArray();

        std::cout << "Added spell " << spell.name.data() << "\n";
        addSpell(spell);
    }
    conf.endArray();
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
        int diff = s->duration - s->timer.elapsed();
        if (diff <= 0) {
            emit sig_sendTimersUpdateToUser(
                        QString("--[ Countdown timer %1 < %2 > finished.\r\n")
                            .arg(s->name, s->desc)
                        );
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


// -----

void CTimers::addSpell(QByteArray spellname, QByteArray up, QByteArray down, QByteArray refresh, bool addon)
{
    TSpell spell;

    spell.name = spellname;
    spell.up_mes = up;
    spell.down_mes = down;
    spell.refresh_mes = refresh;
    spell.addon = addon;
    spell.up = false;
    spell.silently_up = false;

    spells.push_back(spell);
    // setConfigModified(true);
}

void CTimers::addSpell(const TSpell &spell)
{
    spells.push_back(spell);
    // setConfigModified(true);
}

QString CTimers::spellUpFor(unsigned int p)
{
    if (p > spells.size())
        return "";


    QString s;
    int min;
    int sec;

    sec = spells[p].timer.elapsed() / (1000);
    min = sec / 60;
    sec = sec % 60;

    s = QString("- %1%2:%3%4")
            .arg( min / 10 )
            .arg( min % 10 )
            .arg( sec / 10 )
            .arg( sec % 10 );

    return s;
}

void CTimers::resetSpells()
{
    for (unsigned int p = 0; p < spells.size(); p++) {
        spells[p].up = false;
        spells[p].silently_up = false;
    }

}

void CTimers::updateSpellsState(QByteArray line)
{
    for (unsigned int p = 0; p < spells.size(); p++) {
        if ( (spells[p].up_mes != "" && spells[p].up_mes == line) ||
            (spells[p].refresh_mes != "" && spells[p].refresh_mes == line )) {
            spells[p].timer.start();   // start counting
            spells[p].up = true;
            spells[p].silently_up = false;
            // fixme: notify group manager
            // proxy->sendSpellsUpdatedEvent();
            break;
        }

        // if some spell is up - only then we check if its down
        if (spells[p].up && spells[p].down_mes != "" && spells[p].down_mes == line) {
            spells[p].up = false;
            spells[p].silently_up = false;
            // fixme: notify group manager
            // proxy->sendSpellsUpdatedEvent();
            break;
        }
    }
}

QByteArray CTimers::checkAffectedByLine(QByteArray line)
{
    if ((line.length() > 3)) {
        for (unsigned int p = 0; p < spells.size(); p++) {
            if (line.indexOf(spells[p].name) == 2) {
                QString s;
                if (spells[p].up) {
                    s = QString("- %1 (up for %2)")
                            .arg( (const char *)spells[p].name )
                            .arg( spellUpFor(p) );
                } else {
                    s = QString("- %1 (unknown time)")
                            .arg( (const char *)spells[p].name );
                    spells[p].silently_up = true;
                    // fixme: update group manager
                    // proxy->sendSpellsUpdatedEvent();
                }

                return qPrintable(s);
            }
        }
    }

    return line;
}
