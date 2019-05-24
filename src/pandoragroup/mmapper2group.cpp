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
#include <QDateTime>
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
#include "mmapper2character.h"

static constexpr const bool THREADED = true;
static constexpr const auto ONE_MINUTE = 60;
static constexpr const int THIRTY_MINUTES = 30 * ONE_MINUTE;
const Mmapper2Group::AffectTimeout Mmapper2Group::s_affectTimeout
    = {{CharacterAffect::BASHED, 4},
       {CharacterAffect::BLIND, THIRTY_MINUTES},
       {CharacterAffect::POISONED, 5 * ONE_MINUTE},
       {CharacterAffect::SLEPT, THIRTY_MINUTES},
       {CharacterAffect::BLEEDING, 2 * ONE_MINUTE},
       {CharacterAffect::HUNGRY, 2 * ONE_MINUTE},
       {CharacterAffect::THIRSTY, 2 * ONE_MINUTE}};

Mmapper2Group::Mmapper2Group(QObject *const /* parent */)
    : QObject(nullptr)
    , affectTimer{this}
    , networkLock(QMutex::Recursive)
    , thread(THREADED ? new QThread : nullptr)
{
    qRegisterMetaType<CharacterPosition>("CharacterPosition");
    qRegisterMetaType<CharacterAffect>("CharacterAffect");

    connect(this,
            &Mmapper2Group::sig_invokeStopInternal,
            this,
            &Mmapper2Group::slot_stopInternal,
            Qt::ConnectionType::BlockingQueuedConnection);

    affectTimer.setInterval(1);
    connect(&affectTimer, &QTimer::timeout, this, &Mmapper2Group::onAffectTimeout);

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

    affectTimer.start(1);
    emit updateWidget();
    return true;
}

void Mmapper2Group::slot_stopInternal()
{
    assert(QThread::currentThread() == QObject::thread());
    assert(QThread::currentThread() == Mmapper2Group::thread.get() || !Mmapper2Group::thread);
    affectTimer.stop();
    stopNetwork();
    ++m_calledStopInternal;
}

void Mmapper2Group::stop()
{
    emit sig_invokeStopInternal();
    assert(m_calledStopInternal > 0);

    // Wait until the thread is halted
    if (thread && thread->isRunning()) {
        thread->quit();
        thread->wait(1000);
        thread.release(); // finished() and destroyed() signals will destruct the thread
    }
}

void Mmapper2Group::characterChanged()
{
    emit updateWidget();
    emit updateMapCanvas();
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

    issueLocalCharUpdate();
}

void Mmapper2Group::setCharacterRoomId(RoomId roomId)
{
    QMutexLocker locker(&networkLock);
    if (!group) {
        return;
    }

    if (group->getSelf()->getRoomId() != roomId) {
        group->getSelf()->setRoomId(roomId);
        issueLocalCharUpdate();
    }
}

void Mmapper2Group::issueLocalCharUpdate()
{
    emit updateWidget();

    QMutexLocker locker(&networkLock);
    if (!group || getMode() == GroupManagerState::Off) {
        return;
    }

    if (network) {
        const QVariantMap &data = group->getSelf()->toVariantMap();
        network->sendCharUpdate(data);
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

void Mmapper2Group::parseScoreInformation(const QByteArray &score)
{
    if (!group)
        return;
    static const QRegularExpression sRx(R"(^(\d+)\/(\d+) hits)"         // Group 1/2 hits
                                        R"(,?(?: (\d+)\/(\d+) mana,)?)" // Group 3/4 mana
                                        R"( and (\d+)\/(\d+) moves.)"); // Group 5/6 moves

    QRegularExpressionMatch match = sRx.match(score);
    if (!match.hasMatch())
        return;
    const int hp = match.captured(1).toInt();
    const int maxhp = match.captured(2).toInt();
    const int mana = match.captured(3).toInt();
    const int maxmana = match.captured(4).toInt();
    const int moves = match.captured(5).toInt();
    const int maxmoves = match.captured(6).toInt();

    CGroupChar *self = getGroup()->getSelf();
    if (self->hp == hp && self->maxhp == maxhp && self->mana == mana && self->maxmana == maxmana
        && self->moves == moves && self->maxmoves == maxmoves)
        return; // No update needed

    emit log("GroupManager",
             QString("Updated score: %1/%2 hits, %3/%4 mana, and %5/%6 moves.")
                 .arg(hp)
                 .arg(maxhp)
                 .arg(mana)
                 .arg(maxmana)
                 .arg(moves)
                 .arg(maxmoves));

    self->setScore(hp, maxhp, mana, maxmana, moves, maxmoves);

    issueLocalCharUpdate();
}

void Mmapper2Group::parsePromptInformation(const QByteArray &prompt)
{
    if (!group)
        return;

    CGroupChar *self = getGroup()->getSelf();
    QByteArray textHP{}, textMana{}, textMoves{};

    static const QRegularExpression pRx(R"(^)"
                                        R"(([\*@\!\)o])"            // Group 1: light
                                        R"([\[#\.f\(<%~WU\+:=O]?))" //          terrain
                                        R"(([~'"]?[-=]?)"           // Group 2: weather
                                        R"( ?[Cc]?[Rr]?[Ss]?W?)"    //          movement
                                        R"((?: i[^ >]+)?)"          //          wizinvis
                                        R"((?: NN)?(?: NS)?)"       //          no narrate / no song
                                        R"((?: \d+\[\d+:\d+\])?)?)" //          god zone / room info
                                        R"((?: HP:([^ >]+))?)"      // Group 3: HP
                                        R"((?: Mana:([^ >]+))?)"    // Group 4: Mana
                                        R"((?: Move:([^ >]+))?)"    // Group 5: Move
                                        R"((?: Mount:([^ >]+))?)"   // Group 6: Mount
                                        R"((?: ([^>:]+):([^ >]+))?)" // Group 7/8: Target / health
                                        R"((?: ([^>:]+):([^ >]+))?)" // Group 9/10: Buffer / health
                                        R"(>)");
    QRegularExpressionMatch match = pRx.match(prompt);
    if (!match.hasMatch())
        return;

    // REVISIT: Use remaining captures for more purposes and move this code to parser (?)
    textHP = match.captured(3).toLatin1();
    textMana = match.captured(4).toLatin1();
    textMoves = match.captured(5).toLatin1();
    bool inCombat = !match.captured(7).isEmpty();

    if (textHP == lastPrompt.textHP && textMana == lastPrompt.textMana
        && textMoves == lastPrompt.textMoves && inCombat == lastPrompt.inCombat)
        return; // No update needed

    if (textHP == "Dying" || self->position == CharacterPosition::INCAPACITATED) {
        // Incapacitated state overrides fighting state
        self->position = CharacterPosition::INCAPACITATED;

    } else if (inCombat) {
        self->position = CharacterPosition::FIGHTING;

    } else if (self->position != CharacterPosition::DEAD) {
        // Recover standing state if we're done fighting (unless we died because we're dead)
        if (lastPrompt.inCombat) {
            self->position = CharacterPosition::STANDING;
        }
    }

    // Update last prompt values
    lastPrompt.textHP = textHP;
    lastPrompt.textMana = textMana;
    lastPrompt.textMoves = textMoves;
    lastPrompt.inCombat = inCombat;

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
            X_SCORE("Fine", 0.71, 0.99);
            X_SCORE("Hurt", 0.46, 0.70);
            X_SCORE("Wounded", 0.26, 0.45);
            X_SCORE("Bad", 0.11, 0.25);
            X_SCORE("Awful", 0.01, 0.10);
            return 0.0; // Dying
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

void Mmapper2Group::updateCharacterPosition(CharacterPosition position)
{
    if (!group)
        return;

    CharacterPosition &oldPosition = group->getSelf()->position;

    if (oldPosition == position)
        return; // No update needed

    // Reset affects on death
    if (position == CharacterPosition::DEAD)
        group->getSelf()->affects = CharacterAffects{};

    if (oldPosition == CharacterPosition::DEAD && position != CharacterPosition::STANDING)
        return; // Prefer dead state until we finish recovering some hp (i.e. stand)

    oldPosition = position;
    issueLocalCharUpdate();
}

void Mmapper2Group::updateCharacterAffect(CharacterAffect affect, bool enable)
{
    if (!group)
        return;

    if (enable)
        affectLastSeen.insert(affect, QDateTime::QDateTime::currentDateTimeUtc().toTime_t());

    CharacterAffects &affects = getGroup()->self->affects;
    if (enable == affects.contains(affect))
        return; // No update needed

    if (enable) {
        affects.insert(affect);
    } else {
        affects.remove(affect);
        affectLastSeen.remove(affect);
    }

    issueLocalCharUpdate();
}

void Mmapper2Group::onAffectTimeout()
{
    if (affectLastSeen.isEmpty())
        return;

    bool removedAtLeastOneAffect = false;
    CharacterAffects &affects = getGroup()->self->affects;
    const auto now = QDateTime::QDateTime::currentDateTimeUtc().toTime_t();
    for (CharacterAffect affect : affectLastSeen.keys()) {
        const auto lastSeen = affectLastSeen.value(affect, 0);
        const int elapsed = static_cast<int>(now - lastSeen);
        const int timeout = s_affectTimeout.value(affect, THIRTY_MINUTES);
        if (elapsed > timeout) {
            removedAtLeastOneAffect = true;
            affects.remove(affect);
            affectLastSeen.remove(affect);
        }
    }
    if (removedAtLeastOneAffect)
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
    lastPrompt.inCombat = false;

    // Reset character
    CGroupChar *self = getGroup()->getSelf();
    self->hp = 0;
    self->maxhp = 0;
    self->mana = 0;
    self->maxmana = 0;
    self->moves = 0;
    self->maxmoves = 0;
    self->roomId = DEFAULT_ROOMID;
    self->position = CharacterPosition::UNDEFINED;
    self->affects = CharacterAffects{};
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
        emit updateWidget();

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
