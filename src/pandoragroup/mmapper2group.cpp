/************************************************************************
**
** Authors:   Azazello <lachupe@gmail.com>,
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara)
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#include "mmapper2group.h"

#include <QColor>
#include <QMessageLogContext>
#include <QMutex>
#include <QVariantMap>
#include <QtCore>

#include "../configuration/configuration.h"
#include "../expandoracommon/component.h"
#include "../global/roomid.h"
#include "CGroup.h"
#include "CGroupChar.h"
#include "CGroupCommunicator.h"

Mmapper2Group::Mmapper2Group()
    : Component(true)
    , networkLock(QMutex::Recursive)
{}

Mmapper2Group::~Mmapper2Group()
{
    group.reset(nullptr);
    if (network) {
        network->disconnectCommunicator();
        network.reset(nullptr);
    }
}

void Mmapper2Group::init()
{
    group.reset(new CGroup(this));

    connect(group.get(), &CGroup::log, this, &Mmapper2Group::sendLog);
    connect(group.get(),
            &CGroup::characterChanged,
            this,
            &Mmapper2Group::characterChanged,
            Qt::QueuedConnection);

    emit log("GroupManager", "Starting up the GroupManager");
}

void Mmapper2Group::characterChanged()
{
    if (getType() != GroupManagerState::Off) {
        emit drawCharacters();
    }
}

void Mmapper2Group::updateSelf()
{
    QMutexLocker locker(&networkLock);
    if (!group) {
        return;
    }

    const Configuration::GroupManagerSettings &groupManagerSettings = getConfig().groupManager;
    if (group->getSelf()->getName() != groupManagerSettings.charName) {
        QByteArray oldname = group->getSelf()->getName();
        QByteArray newname = groupManagerSettings.charName;

        if (network) {
            network->renameConnection(oldname, newname);
        }
        group->getSelf()->setName(newname);

    } else if (group->getSelf()->getColor() != groupManagerSettings.color) {
        group->getSelf()->setColor(groupManagerSettings.color);

    } else {
        return;
    }

    if (getType() != GroupManagerState::Off) {
        issueLocalCharUpdate();
    }
}

void Mmapper2Group::setCharPosition(RoomId pos)
{
    QMutexLocker locker(&networkLock);
    if (!group || getType() == GroupManagerState::Off) {
        return;
    }

    if (group->getSelf()->getPosition() != pos) {
        group->getSelf()->setPosition(pos);
        issueLocalCharUpdate();
    }
}

void Mmapper2Group::issueLocalCharUpdate()
{
    QMutexLocker locker(&networkLock);
    if (!group || getType() == GroupManagerState::Off) {
        return;
    }

    if (network) {
        const QVariantMap &data = group->getSelf()->toVariantMap();
        network->sendCharUpdate(data);
        emit drawCharacters();
    }
}

void Mmapper2Group::relayMessageBox(const QString &message)
{
    emit log("GroupManager", message.toLatin1());
    emit messageBox("Group Manager", message);
}

void Mmapper2Group::serverStartupFailed(const QString &message)
{
    relayMessageBox(
        QString("Failed to start the Group server: %1").arg(message.toLatin1().constData()));
}

void Mmapper2Group::gotKicked(const QVariantMap &message)
{
    if (!message.contains("text") && message["text"].canConvert(QMetaType::QString)) {
        qWarning() << "Text not found" << message;
        return;
    }
    const QString &text = message["text"].toString();
    const QString reason = QString("You got kicked! Reason: %1").arg(text);
    emit log("GroupManager", reason);
    emit messageBox("Group Manager", reason);
}

void Mmapper2Group::gTellArrived(const QVariantMap &node)
{
    if (!node.contains("from") && node["from"].canConvert(QMetaType::QByteArray)) {
        qWarning() << "From not found" << node;
        return;
    }
    const QByteArray &from = node["from"].toByteArray();

    if (!node.contains("text") && node["text"].canConvert(QMetaType::QByteArray)) {
        qWarning() << "Text not found" << node;
        return;
    }

    const QString &text = node["text"].toString();
    emit log("GroupManager", QString("GTell from %1 arrived: %2").arg(from.constData()).arg(text));

    // TODO: Color
    const QByteArray tell
        = QString("%1 tells you [GT] '%2'").arg(from.constData()).arg(text).toLatin1();

    emit displayGroupTellEvent(tell);
}

void Mmapper2Group::sendGroupTell(const QByteArray &tell)
{
    QMutexLocker locker(&networkLock);
    if (network) {
        network->sendGroupTell(tell);
    }
}

void Mmapper2Group::parseScoreInformation(QByteArray score)
{
    if (!group || getType() == GroupManagerState::Off) {
        return;
    }
    emit log("GroupManager", QString("Caught a score line: %1").arg(score.constData()));

    if (score.contains("mana, ")) {
        score.replace(" hits, ", "/");
        score.replace(" mana, and ", "/");
        score.replace(" moves.", "");

        QString temp = score;
        QStringList list = temp.split('/');

        /*
        qDebug( "Hp: %s", (const char *) list[0].toLatin1());
        qDebug( "Hp max: %s", (const char *) list[1].toLatin1());
        qDebug( "Mana: %s", (const char *) list[2].toLatin1());
        qDebug( "Max Mana: %s", (const char *) list[3].toLatin1());
        qDebug( "Moves: %s", (const char *) list[4].toLatin1());
        qDebug( "Max Moves: %s", (const char *) list[5].toLatin1());
        */

        group->getSelf()->setScore(list[0].toInt(),
                                   list[1].toInt(),
                                   list[2].toInt(),
                                   list[3].toInt(),
                                   list[4].toInt(),
                                   list[5].toInt());

    } else {
        // 399/529 hits and 121/133 moves.
        score.replace(" hits and ", "/");
        score.replace(" moves.", "");

        QString temp = score;
        QStringList list = temp.split('/');

        /*
        qDebug( "Hp: %s", (const char *) list[0].toLatin1());
        qDebug( "Hp max: %s", (const char *) list[1].toLatin1());
        qDebug( "Moves: %s", (const char *) list[2].toLatin1());
        qDebug( "Max Moves: %s", (const char *) list[3].toLatin1());
        */
        group->getSelf()
            ->setScore(list[0].toInt(), list[1].toInt(), 0, 0, list[2].toInt(), list[3].toInt());
    }
    issueLocalCharUpdate();
}

void Mmapper2Group::parsePromptInformation(QByteArray prompt)
{
    if (!group || getType() == GroupManagerState::Off) {
        return;
    }

    if (!prompt.contains('>')) {
        return; // false prompt
    }

    CGroupLocalChar *self = group->getSelf();
    QByteArray oldHP = self->textHP;
    QByteArray oldMana = self->textMana;
    QByteArray oldMoves = self->textMoves;

    QByteArray hp = oldHP, mana = oldMana, moves = oldMoves;

    int next = 0;
    int index = prompt.indexOf("HP:", next);
    if (index != -1) {
        hp = "";
        int k = index + 3;
        while (prompt[k] != ' ' && prompt[k] != '>') {
            hp += prompt[k++];
        }
        next = k;
    }

    index = prompt.indexOf("Mana:", next);
    if (index != -1) {
        mana = "";
        int k = index + 5;
        while (prompt[k] != ' ' && prompt[k] != '>') {
            mana += prompt[k++];
        }
        next = k;
    }

    index = prompt.indexOf("Move:", next);
    if (index != -1) {
        moves = "";
        int k = index + 5;
        while (prompt[k] != ' ' && prompt[k] != '>') {
            moves += prompt[k++];
        }
    }

    if (hp != oldHP || mana != oldMana || moves != oldMoves) {
        self->setTextScore(hp, mana, moves);

        // Estimate new numerical scores using prompt
        double maxhp = self->maxhp;
        double maxmoves = self->maxmoves;
        double maxmana = self->maxmana;
        if (!hp.isEmpty() && maxhp != 0) {
            // NOTE: This avoids capture so it can be lifted out more easily later.
            const auto calc_hp = [](const QByteArray &hp, const double maxhp) -> double {
                if (hp == "Healthy") {
                    return maxhp;
                } else if (hp == "Fine") {
                    return maxhp * 0.99;
                } else if (hp == "Hurt") {
                    return maxhp * 0.65;
                } else if (hp == "Wounded") {
                    return maxhp * 0.45;
                } else if (hp == "Bad") {
                    return maxhp * 0.25;
                } else if (hp == "Awful") {
                    return maxhp * 0.10;
                } else {
                    // Incap
                    return 0.0;
                }
            };
            const auto newhp = static_cast<int>(calc_hp(hp, maxhp));
            if (self->hp > newhp) {
                self->hp = newhp;
            }
        }
        if (!mana.isEmpty() && maxmana != 0) {
            const auto calc_mana = [](const QByteArray &mana, const double maxmana) -> double {
                if (mana == "Burning") {
                    return maxmana * 0.99;
                } else if (mana == "Hot") {
                    return maxmana * 0.75;
                } else if (mana == "Warm") {
                    return maxmana * 0.45;
                } else if (mana == "Cold") {
                    return maxmana * 0.25;
                } else if (mana == "Icy") {
                    return maxmana * 0.10;
                } else {
                    // Frozen
                    return 0.0;
                }
            };
            const auto newmana = static_cast<int>(calc_mana(mana, maxmana));
            if (self->mana > newmana) {
                self->mana = newmana;
            }
        }
        if (!moves.isEmpty() && maxmoves != 0) {
            const auto calc_moves =
                [](const QByteArray &moves, const double maxmoves, const int /*def*/) -> double {
                if (moves == "Tired") {
                    return maxmoves * 0.42;
                } else if (moves == "Slow") {
                    return maxmoves * 0.31;
                } else if (moves == "Weak") {
                    return maxmoves * 0.12;
                } else if (moves == "Fainting") {
                    return maxmoves * 0.05;
                } else {
                    // Exhausted
                    return 0.0;
                }
            };
            const auto newmoves = static_cast<int>(calc_moves(moves, maxmoves, self->moves));
            if (self->moves > newmoves) {
                self->moves = newmoves;
            }
        }

        issueLocalCharUpdate();
    }
}

void Mmapper2Group::sendLog(const QString &text)
{
    emit log("GroupManager", text);
}

GroupManagerState Mmapper2Group::getType()
{
    QMutexLocker locker(&networkLock);
    return network ? network->getType() : GroupManagerState::Off;
}

void Mmapper2Group::networkDown()
{
    qDebug() << "Network down";
    emit groupManagerOff();
}

void Mmapper2Group::setType(GroupManagerState newState)
{
    QMutexLocker locker(&networkLock);

    // Delete previous network and regenerate
    if (network) {
        network->disconnectCommunicator();
        network->deleteLater();
        network.release(); // There might still be signals left to be processed
    }
    switch (newState) {
    case GroupManagerState::Server:
        network.reset(new CGroupServerCommunicator(this));
        break;
    case GroupManagerState::Client:
        network.reset(new CGroupClientCommunicator(this));
        break;
    case GroupManagerState::Off:
    default:
        sendLog("Off mode has been selected");
        break;
    }

    qDebug() << "Network type set to" << static_cast<int>(newState);
    setConfig().groupManager.state = newState;

    if (newState != GroupManagerState::Off) {
        if (getConfig().groupManager.rulesWarning) {
            emit messageBox("Warning: MUME Rules",
                            "Using the GroupManager in PK situations is ILLEGAL "
                            "according to RULES ACTIONS.\n\nBe sure to disable the "
                            "GroupManager under such conditions.");
        }
        emit drawCharacters();
    }
}
