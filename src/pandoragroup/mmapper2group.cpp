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
#include "CGroup.h"
#include "CGroupChar.h"
#include "CGroupCommunicator.h"
#include "configuration.h"

#include <assert.h>
#include <QDebug>
#include <QMutex>

#ifdef MODULAR
extern "C" MY_EXPORT Component *createComponent()
{
    return new Mmapper2Group;
}
#else
Initializer<Mmapper2Group> mmapper2Group("mmapper2Group");
#endif

Mmapper2Group::Mmapper2Group() :
    Component(true),
    networkLock(QMutex::Recursive),
    network(NULL),
    group(NULL)
{}

Mmapper2Group::~Mmapper2Group()
{
    if (group) {
        delete group;
    }
    if (network) {
        network->disconnect();
        delete network;
    }
}

void Mmapper2Group::init()
{
    group = new CGroup(this);

    connect(group, SIGNAL(characterChanged()), SLOT(characterChanged()), Qt::QueuedConnection);

    emit log("GroupManager", "Starting up the GroupManager");
}

void Mmapper2Group::characterChanged()
{
    if (getType() != Off) {
        emit drawCharacters();
    }
}

void Mmapper2Group::updateSelf()
{
    QMutexLocker locker(&networkLock);
    if (!getGroup()) return;

    if (getGroup()->getSelf()->getName() != Config().m_groupManagerCharName) {
        QByteArray oldname = getGroup()->getSelf()->getName();
        QByteArray newname = Config().m_groupManagerCharName;

        if (network) network->renameConnection(oldname, newname);
        getGroup()->getSelf()->setName(newname);

    } else if (getGroup()->getSelf()->getColor() != Config().m_groupManagerColor ) {
        getGroup()->getSelf()->setColor(Config().m_groupManagerColor);

    } else {
        return;
    }

    if (getType() != Off)
        issueLocalCharUpdate();
}

void Mmapper2Group::setCharPosition(unsigned int pos)
{
    QMutexLocker locker(&networkLock);
    if (!getGroup() || getType() == Off) return;

    if (getGroup()->getSelf()->getPosition() != pos) {
        getGroup()->getSelf()->setPosition(pos);
        issueLocalCharUpdate();
    }
}

void Mmapper2Group::issueLocalCharUpdate()
{
    QMutexLocker locker(&networkLock);
    if (!getGroup() || getType() == Off) return;

    if (network) {
        QDomNode data = getGroup()->getSelf()->toXML();
        network->sendCharUpdate(data);
        emit drawCharacters();
    }
}

void Mmapper2Group::relayMessageBox(QString message)
{
    emit log("GroupManager", message.toLatin1());
    emit messageBox("Group Manager", message);
}

void Mmapper2Group::serverStartupFailed(QString message)
{
    relayMessageBox(QString("Failed to start the Group server: %1")
                    .arg(message.toLatin1().constData()));
}

void Mmapper2Group::gotKicked(QDomNode message)
{
    if (message.nodeName() != "data") {
        qWarning() << "Called gotKicked with wrong node. No data node but instead:" << message.nodeName();
        return;
    }

    QDomNode e = message.firstChildElement();

    if (e.nodeName() != "text") {
        qWarning() << "Called gotKicked with wrong node. No text node but instead:" << e.nodeName();
        return;
    }

    QDomElement text = e.toElement();
    emit log("GroupManager", QString("You got kicked! Reason [nodename %1] : %2")
             .arg(text.nodeName().toLatin1().constData())
             .arg(text.text().toLatin1().constData()));
    emit messageBox("Group Manager", QString("You got kicked! Reason: %1.").arg(text.text()));

}

void Mmapper2Group::gTellArrived(QDomNode node)
{
    if (node.nodeName() != "data") {
        qWarning() << "Called gTellArrived with wrong node. No data node but instead:" << node.nodeName();
        return;
    }

    QDomNode e = node.firstChildElement();
    QString from = e.toElement().attribute("from");

    if (e.nodeName() != "gtell") {
        qWarning() << "Called gTellArrived with wrong node. No text node but instead:" << e.nodeName();
        return;
    }

    QDomElement text = e.toElement();
    emit log("GroupManager", QString("GTell from %1, Arrived : %2").arg(
                 from.toLatin1().constData()).arg(text.text().toLatin1().constData()));

    QByteArray tell = QString("" + from + " tells you [GT] '" + text.text() + "'\r\n").toLatin1();

    emit displayGroupTellEvent(tell);
}

void Mmapper2Group::sendGTell(QByteArray tell)
{
    QMutexLocker locker(&networkLock);
    if (network) network->sendGTell(tell);
}

void Mmapper2Group::parseScoreInformation(QByteArray score)
{
    if (!getGroup() || getType() == Off) return;
    emit log("GroupManager", QString("Caught a score line: %1").arg(score.constData()));

    if (score.contains("mana, ") == true) {
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

        getGroup()->getSelf()->setScore(list[0].toInt(), list[1].toInt(), list[2].toInt(), list[3].toInt(),
                                        list[4].toInt(), list[5].toInt());

        issueLocalCharUpdate();

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
        getGroup()->getSelf()->setScore(list[0].toInt(), list[1].toInt(), 0, 0,
                                        list[2].toInt(), list[3].toInt());

        issueLocalCharUpdate();
    }
}

void Mmapper2Group::parsePromptInformation(QByteArray prompt)
{
    if (!getGroup() || getType() == Off) return;

    if (!prompt.contains('>'))
        return; // false prompt

    CGroupChar *self = getGroup()->getSelf();
    QByteArray oldHP = self->textHP;
    QByteArray oldMana = self->textMana;
    QByteArray oldMoves = self->textMoves;

    QByteArray hp, mana, moves;
    hp = oldHP;
    mana = oldMana;
    moves = oldMoves;

    int index = prompt.indexOf("HP:");
    if (index != -1) {
        hp = "";
        int k = index + 3;
        while (prompt[k] != ' ' && prompt[k] != '>' )
            hp += prompt[k++];
    }

    index = prompt.indexOf("Mana:");
    if (index != -1) {
        mana = "";
        int k = index + 5;
        while (prompt[k] != ' ' && prompt[k] != '>' )
            mana += prompt[k++];
    }

    index = prompt.indexOf("Move:");
    if (index != -1) {
        moves = "";
        int k = index + 5;
        while (prompt[k] != ' ' && prompt[k] != '>' )
            moves += prompt[k++];
    }

    self->setTextScore(hp, mana, moves);

    if (hp != oldHP || mana != oldMana || moves != oldMoves)
        emit issueLocalCharUpdate();
}

void Mmapper2Group::sendLog(const QString &text)
{
    emit log("GroupManager", text);
}

int Mmapper2Group::getType()
{
    QMutexLocker locker(&networkLock);
    if (network != NULL) return network->getType();
    return 0;
}

void Mmapper2Group::networkDown()
{
    setType(Off);
    emit groupManagerOff();
}

void Mmapper2Group::setType(int newState)
{
    QMutexLocker locker(&networkLock);

    // Delete previous network and regenerate
    if (network) {
        network->disconnect();
        delete network;
        network = NULL;
    }
    switch ((GroupManagerState)newState) {
    case Server:
        network = new CGroupServerCommunicator(this);
        break;
    case Client:
        network = new CGroupClientCommunicator(this);
        break;
    case Off:
    default:
        emit sendLog("Off mode has been selected");
        break;
    }

    qDebug() << "Network type set to" << newState;
    Config().m_groupManagerState = newState;

    if (newState != Off) {
        if (Config().m_groupManagerRulesWarning)
            emit messageBox("Warning: MUME Rules",
                            "Using the GroupManager in PK situations is ILLEGAL "
                            "according to RULES ACTIONS.\n\nBe sure to disable the "
                            "GroupManager under such conditions.");
        emit drawCharacters();
    }
}
