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
#include <QThread>
#include <QVariantMap>
#include <QtCore>

#include "../configuration/configuration.h"
#include "../global/Color.h"
#include "../global/roomid.h"
#include "../parser/CommandQueue.h"
#include "CGroup.h"
#include "CGroupChar.h"
#include "GroupClient.h"
#include "GroupServer.h"
#include "groupauthority.h"
#include "groupselection.h"

static constexpr const bool THREADED = true;

Mmapper2Group::Mmapper2Group(QObject *const /* parent */)
    : QObject(nullptr)
    , networkLock(QMutex::Recursive)
    , thread(THREADED ? new QThread : nullptr)
{
    if (thread) {
        connect(thread.get(), &QThread::started, this, [this]() {
            emit log("GroupManager", "Initialized Group Manager service");
        });
        connect(thread.get(), &QThread::finished, this, [this]() {
            thread.get()->deleteLater();
            qInfo() << "Group Manager thread stopped";
        });
        connect(thread.get(), &QThread::destroyed, this, [this]() {
            thread.release();
            deleteLater();
        });
    }
}

Mmapper2Group::~Mmapper2Group()
{
    // Stop the network
    stop();

    // Release resources
    authority.reset(nullptr);
    group.reset(nullptr);
    network.reset(nullptr);
    thread.reset(nullptr);

    qInfo() << "Terminated Group Manager service";
}

void Mmapper2Group::start()
{
    if (thread != nullptr) {
        if (init()) {
            moveToThread(thread.get());
        }
        thread->start();
    } else {
        init();
    }
}

bool Mmapper2Group::init()
{
    group.reset(new CGroup(this));
    authority.reset(new GroupAuthority(this));

    connect(group.get(), &CGroup::log, this, &Mmapper2Group::sendLog);
    connect(group.get(),
            &CGroup::characterChanged,
            this,
            &Mmapper2Group::characterChanged,
            Qt::QueuedConnection);
    return true;
}

void Mmapper2Group::stop()
{
    // Call stopNetwork() using the Group thread
    QMetaObject::invokeMethod(this, "stopNetwork", Qt::BlockingQueuedConnection);

    // Wait until the thread is halted
    if (thread && thread->isRunning()) {
        thread->quit();
        thread->wait(1000);
        thread.release(); // finished() and destroyed() signals will destruct the thread
    }
}

void Mmapper2Group::characterChanged()
{
    if (getMode() != GroupManagerState::Off) {
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

        if (getGroup()->isNamePresent(newname)) {
            emit messageBox("Group Manager", "You cannot take a name that is already present.");
            setConfig().groupManager.charName = oldname;
            return;
        }

        if (network) {
            network->sendSelfRename(oldname, newname);
        }
        group->getSelf()->setName(newname);

    } else if (group->getSelf()->getColor() != groupManagerSettings.color) {
        group->getSelf()->setColor(groupManagerSettings.color);

    } else {
        return;
    }

    if (getMode() != GroupManagerState::Off) {
        issueLocalCharUpdate();
    }
}

void Mmapper2Group::setCharPosition(RoomId pos)
{
    QMutexLocker locker(&networkLock);
    if (!group || getMode() == GroupManagerState::Off) {
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
    if (!group || getMode() == GroupManagerState::Off) {
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

    auto color = getConfig().groupManager.groupTellColor;
    auto selection = getGroup()->selectByName(from);
    if (getConfig().groupManager.useGroupTellAnsi256Color && !selection->empty()) {
        auto character = selection->at(0);
        color = rgbToAnsi256String(character->getColor(), false);
    }
    emit log("GroupManager", QString("GTell from %1 arrived: %2").arg(from.constData()).arg(text));

    const QByteArray tell = QString("\x1b%1%2 tells you [GT] '%3'\x1b[0m")
                                .arg(color)
                                .arg(from.constData())
                                .arg(text)
                                .toLatin1();

    emit displayGroupTellEvent(tell);
}

void Mmapper2Group::kickCharacter(const QByteArray &character)
{
    QMutexLocker locker(&networkLock);
    if (network) {
        network->kickCharacter(character);
    }
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
    if (!group || getMode() == GroupManagerState::Off) {
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
    if (!group || getMode() == GroupManagerState::Off) {
        return;
    }

    if (!prompt.contains('>')) {
        return; // false prompt
    }

    CGroupChar *self = getGroup()->getSelf();
    QByteArray textHP{}, textMana{}, textMoves{};

    int next = 0;
    int index = prompt.indexOf("HP:", next);
    if (index != -1) {
        int k = index + 3;
        while (prompt[k] != ' ' && prompt[k] != '>') {
            textHP += prompt[k++];
        }
        next = k;
    }

    index = prompt.indexOf("Mana:", next);
    if (index != -1) {
        int k = index + 5;
        while (prompt[k] != ' ' && prompt[k] != '>') {
            textMana += prompt[k++];
        }
        next = k;
    }

    index = prompt.indexOf("Move:", next);
    if (index != -1) {
        int k = index + 5;
        while (prompt[k] != ' ' && prompt[k] != '>') {
            textMoves += prompt[k++];
        }
    }

    if (textHP == lastPrompt.textHP && textMana == lastPrompt.textMana
        && textMoves == lastPrompt.textMoves)
        return; // No update needed

    // Update last prompt values
    lastPrompt.textHP = textHP;
    lastPrompt.textMana = textMana;
    lastPrompt.textMoves = textMoves;

#define X_SCORE(target, lower, upper) \
    do { \
        if (text == target) { \
            const auto ub = max * upper; \
            const auto lb = max * lower; \
            if (current >= ub) \
                return ub; \
            else if (current <= lb) \
                return lb; \
            else \
                return current; \
        } \
    } while (false)

    // Estimate new numerical scores using prompt
    if (self->maxhp != 0) {
        // NOTE: This avoids capture so it can be lifted out more easily later.
        const auto calc_hp =
            [](const QByteArray &text, const double current, const double max) -> double {
            if (text.isEmpty() || text == "Healthy")
                return max;
            X_SCORE("Fine", 0.66, 0.99);
            X_SCORE("Hurt", 0.45, 0.65);
            X_SCORE("Wounded", 0.26, 0.45);
            X_SCORE("Bad", 0.11, 0.25);
            X_SCORE("Awful", 0.01, 0.10);
            return 0.0; // Incap
        };
        self->hp = static_cast<int>(calc_hp(textHP, self->hp, self->maxhp));
    }
    if (self->maxmana != 0) {
        const auto calc_mana =
            [](const QByteArray &text, const double current, const double max) -> double {
            if (text.isEmpty())
                return max;
            X_SCORE("Burning", 0.76, 0.99);
            X_SCORE("Hot", 0.46, 0.75);
            X_SCORE("Warm", 0.26, 0.45);
            X_SCORE("Cold", 0.11, 0.25);
            X_SCORE("Icy", 0.01, 0.10);
            return 0.0; // Frozen
        };
        self->mana = static_cast<int>(calc_mana(textMana, self->mana, self->maxmana));
    }
    if (self->maxmoves != 0) {
        const auto calc_moves =
            [](const QByteArray &text, const double current, const double max) -> double {
            if (text.isEmpty()) {
                const double lb = max * 0.43;
                if (current <= lb)
                    return lb;
                else
                    return current;
            }
            X_SCORE("Tired", 0.32, 0.42);
            X_SCORE("Slow", 0.13, 0.31);
            X_SCORE("Weak", 0.06, 0.12);
            X_SCORE("Fainting", 0.01, 0.05);
            return 0.0; // Exhausted
        };
        self->moves = static_cast<int>(calc_moves(textMoves, self->moves, self->maxmoves));
    }
#undef X_SCORE

    issueLocalCharUpdate();
}

void Mmapper2Group::setPath(CommandQueue dirs, bool)
{
    group->getSelf()->prespam = std::move(dirs);
}

void Mmapper2Group::reset()
{
    QMutexLocker locker(&networkLock);

    // Reset prompt
    lastPrompt.textHP = {};
    lastPrompt.textMana = {};
    lastPrompt.textMoves = {};

    // Reset character
    CGroupChar *self = getGroup()->getSelf();
    self->hp = 0;
    self->maxhp = 0;
    self->mana = 0;
    self->maxmana = 0;
    self->moves = 0;
    self->maxmoves = 0;
    self->pos = DEFAULT_ROOMID;
    issueLocalCharUpdate();
}

void Mmapper2Group::sendLog(const QString &text)
{
    emit log("GroupManager", text);
}

GroupManagerState Mmapper2Group::getMode()
{
    QMutexLocker locker(&networkLock);
    return network ? network->getMode() : GroupManagerState::Off;
}

void Mmapper2Group::startNetwork()
{
    QMutexLocker locker(&networkLock);

    if (!network) {
        // Create network
        switch (getConfig().groupManager.state) {
        case GroupManagerState::Server:
            network.reset(new GroupServer(this));
            break;
        case GroupManagerState::Client:
            network.reset(new GroupClient(this));
            break;
        case GroupManagerState::Off:
            return;
        }

        connect(network.get(), &CGroupCommunicator::sendLog, this, &Mmapper2Group::sendLog);
        connect(network.get(),
                &CGroupCommunicator::messageBox,
                this,
                &Mmapper2Group::relayMessageBox);
        connect(network.get(),
                &CGroupCommunicator::gTellArrived,
                this,
                &Mmapper2Group::gTellArrived);
        connect(network.get(), &CGroupCommunicator::destroyed, this, [this]() {
            network.release();
            emit networkStatus(false);
        });
        connect(network.get(),
                &CGroupCommunicator::scheduleAction,
                getGroup(),
                &CGroup::scheduleAction);
    }

    // REVISIT: What about if the network is already started?
    if (network->start()) {
        emit networkStatus(true);
        emit drawCharacters();

        if (getConfig().groupManager.rulesWarning) {
            emit messageBox("Warning: MUME Rules",
                            "Using the GroupManager in PK situations is ILLEGAL "
                            "according to RULES ACTIONS.\n\nBe sure to disable the "
                            "GroupManager under such conditions.");
        }
        qDebug() << "Network up";
    } else {
        network->stop();
        qDebug() << "Network failed to start";
    }
}

void Mmapper2Group::stopNetwork()
{
    QMutexLocker locker(&networkLock);
    if (network) {
        network->stop();
        qDebug() << "Network down";
    }
}

void Mmapper2Group::setMode(GroupManagerState newMode)
{
    QMutexLocker locker(&networkLock);

    setConfig().groupManager.state = newMode; // Ensure config matches reality

    const auto currentState = getMode();
    if (currentState == newMode)
        return; // Do not bother changing states if we're already in it

    // Stop previous network if it changed
    stopNetwork();

    qDebug() << "Network type set to" << static_cast<int>(newMode);
}
