/*
* Spells.cpp
*
*  Created on: Mar 13, 2011
*      Author: aza
*/

#include "Spells.h"

Spells::Spells(QObject *parent)
    : QObject(parent)
{
    addSpell("armour",
             "A blue transparent wall slowly appears around you.",
             "Your magic armour is revitalised.",
             "You feel less protected.",
             false);
    addSpell("shield",
             "You feel protected.",
             "Your protection is revitalised.",
             "Your magical shield wears off.",
             false);
    addSpell("strength",
             "You feel stronger.",
             "The duration of the strength spell has been improved.",
             "You feel weaker.",
             false);
    addSpell("bless",
             "You begin to feel the light of Aman shine upon you.",
             "You feel a renewed light shine upon you.",
             "The light of Aman fades away from you.",
             false);
    addSpell("sense life",
             "You feel your awareness improve.",
             "Your awareness is refreshed.",
             "You feel less aware of your surroundings.",
             false);
    addSpell("sanctuary",
             "You start glowing.",
             "Your aura glows more intensely.",
             "The white aura around your body fades.",
             false);
    addSpell("detect magic",
             "You become sensitive of magical auras.",
             "Your awareness of magical auras is renewed.",
             "Your perception of magical auras wears off.",
             false);
    addSpell(
        "tiredness",
        "You feel your muscles relax and your pulse slow as the strength that welled within you subsides.",
        "",
        "You feel your muscles regain some of their former energy.",
        false);
    addSpell(
        "haggardness",
        "You feel a sudden flash of dizziness causing you to pause before getting your directional bearings back.",
        "",
        "You feel steadier now.",
        false);
    addSpell(
        "lethargy",
        "You feel a sudden loss of energy as the power that once mingled with your own has now vanished.",
        "",
        "You feel your magic energy coming back to you.",
        false);
    addSpell("blindness",
             "You have been blinded!",
             "",
             "You feel a cloak of blindness dissolve.",
             false);
    addSpell("nightvision", "Your eyes tingle.", "", "Your vision blurs.", false);
    addSpell("battleglory",
             "Hearing the horn blow, you feel your urge to battle increase!",
             "",
             "You feel your newfound strength leaving you again.",
             false);
    addSpell("breath of briskness",
             "An energy begins to flow within your legs as your body becomes lighter.",
             "The energy in your legs is refreshed.",
             "Your legs feel heavier.",
             false);
}

Spells::~Spells() {}

void Spells::addSpell(
    QByteArray spellName, QByteArray up, QByteArray refresh, QByteArray down, bool addon)
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

QString Spells::spellUpFor(unsigned int p)
{
    if (p > spells.size())
        return "";

    QString s;
    int min;
    int sec;

    sec = spells[p].timer.elapsed() / (1000);
    min = sec / 60;
    sec = sec % 60;

    s = QString("- %1%2:%3%4").arg(min / 10).arg(min % 10).arg(sec / 10).arg(sec % 10);

    return s;
}

void Spells::reset()
{
    for (unsigned int p = 0; p < spells.size(); p++) {
        spells[p].up = false;
        spells[p].silently_up = false;
    }
}

void Spells::updateSpellsState(QByteArray line)
{
    for (unsigned int p = 0; p < spells.size(); p++) {
        if ((spells[p].up_mes != "" && spells[p].up_mes == line)
            || (spells[p].refresh_mes != "" && spells[p].refresh_mes == line)) {
            spells[p].timer.start(); // start counting
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

QByteArray Spells::checkAffectedByLine(QByteArray line)
{
    if ((line.length() > 3)) {
        for (unsigned int p = 0; p < spells.size(); p++) {
            if (line.indexOf(spells[p].name) == 2) {
                QString s;
                if (spells[p].up) {
                    s = QString("- %1 (up for %2)")
                            .arg((const char *) spells[p].name)
                            .arg(spellUpFor(p));
                } else {
                    s = QString("- %1 (unknown time)").arg((const char *) spells[p].name);
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
