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

    addSpell(
        "armour",
        "A blue transparent wall slowly appears around you.",
        "Your magic armour is revitalised.",
        "You feel less protected.",
        false
    );
    addSpell(
        "shield",
        "You feel protected.",
        "Your protection is revitalised.",
        "Your magical shield wears off.",
        false
    );
    addSpell(
        "strength",
        "You feel stronger.",
        "The duration of the strength spell has been improved.",
        "You feel weaker.",
        false
    );
    addSpell(
        "bless",
        "You begin to feel the light of Aman shine upon you.",
        "You feel a renewed light shine upon you.",
        "The light of Aman fades away from you.",
        false
    );
    addSpell(
        "sense life",
        "You feel your awareness improve.",
        "Your awareness is refreshed.",
        "You feel less aware of your surroundings.",
        false
    );
    addSpell(
        "sanctuary",
        "You start glowing.",
        "Your aura glows more intensely.",
        "The white aura around your body fades.",
        false
    );
    addSpell(
        "detect magic",
        "You become sensitive of magical auras.",
        "Your awareness of magical auras is renewed.",
        "Your perception of magical auras wears off.",
        false
    );
    addSpell(
        "tiredness",
        "You feel your muscles relax and your pulse slow as the strength that welled within you subsides.",
        "",
        "You feel your muscles regain some of their former energy.",
        false
    );
    addSpell(
        "haggardness",
        "You feel a sudden flash of dizziness causing you to pause before getting your directional bearings back.",
        "",
        "You feel steadier now.",
        false
    );
    addSpell(
        "lethargy",
        "You feel a sudden loss of energy as the power that once mingled with your own has now vanished.",
        "",
        "You feel your magic energy coming back to you.",
        false
    );
    addSpell(
        "blindness",
        "You have been blinded!",
        "",
        "You feel a cloak of blindness dissolve.",
        false
    );
    addSpell(
        "nightvision",
        "Your eyes tingle.",
        "",
        "Your vision blurs.",
        false
    );
    addSpell(
        "battleglory",
        "Hearing the horn blow, you feel your urge to battle increase!",
        "",
        "You feel your newfound strength leaving you again.",
        false
    );
    addSpell(
        "breath of briskness",
        "An energy begins to flow within your legs as your body becomes lighter.",
        "The energy in your legs is refreshed.",
        "Your legs feel heavier.",
        false
    );
}

CTimers::~CTimers()
{
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

void CTimers::addSpell(QByteArray spellName, QByteArray up, QByteArray refresh, QByteArray down, bool addon)
{
    TSpell spell;

    spell.name = spellName;
    spell.up_mes = up;
    spell.down_mes = down;
    spell.refresh_mes = refresh;
    spell.addon = addon;
    spell.up = false;
    spell.silently_up = false;

    spells.push_back(spell);
}

void CTimers::addSpell(const TSpell &spell)
{
    spells.push_back(spell);
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
