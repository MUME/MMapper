// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "mmapper2group.h"

#include <memory>
#include <QColor>
#include <QDateTime>
#include <QMessageLogContext>
#include <QMutex>
#include <QThread>
#include <QVariantMap>
#include <QtCore>

#include "../configuration/configuration.h"
#include "../global/AnsiColor.h"
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
static constexpr const int DEFAULT_EXPIRE = THIRTY_MINUTES;
const Mmapper2Group::AffectTimeout Mmapper2Group::s_affectTimeout
    = {{CharacterAffectEnum::BASHED, 4},
       {CharacterAffectEnum::BLIND, THIRTY_MINUTES},
       {CharacterAffectEnum::POISONED, 5 * ONE_MINUTE},
       {CharacterAffectEnum::SLEPT, THIRTY_MINUTES},
       {CharacterAffectEnum::BLEEDING, 2 * ONE_MINUTE},
       {CharacterAffectEnum::HUNGRY, 2 * ONE_MINUTE},
       {CharacterAffectEnum::THIRSTY, 2 * ONE_MINUTE}};

Mmapper2Group::Mmapper2Group(QObject *const /* parent */)
    : QObject(nullptr)
    , affectTimer{this}
    , networkLock(QMutex::Recursive)
    , thread(THREADED ? new QThread : nullptr)
{
    qRegisterMetaType<CharacterPositionEnum>("CharacterPositionEnum");
    qRegisterMetaType<CharacterAffectEnum>("CharacterAffectEnum");

    connect(this,
            &Mmapper2Group::sig_invokeStopInternal,
            this,
            &Mmapper2Group::slot_stopInternal,
            Qt::ConnectionType::BlockingQueuedConnection);

    affectTimer.setInterval(1000);
    affectTimer.setSingleShot(false);
    affectTimer.start();
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
    authority.reset();
    group.reset();
    network.reset();
    thread.reset();

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
    group = std::make_unique<CGroup>(this);
    authority = std::make_unique<GroupAuthority>(this);

    connect(group.get(), &CGroup::log, this, &Mmapper2Group::sendLog);
    connect(group.get(),
            &CGroup::characterChanged,
            this,
            &Mmapper2Group::characterChanged,
            Qt::QueuedConnection);

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

void Mmapper2Group::characterChanged(bool updateCanvas)
{
    emit updateWidget();
    if (updateCanvas)
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
            emit sig_sendSelfRename(oldname, newname);
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
    if (!group)
        return;

    if (group->getSelf()->getRoomId() == roomId)
        return; // No update needed

    // Check if we are still snared
    static const constexpr auto SNARED_MESSAGE_WINDOW = 1;
    CharacterAffects &affects = getGroup()->getSelf()->affects;
    if (affects.contains(CharacterAffectEnum::SNARED)) {
        const int64_t now = QDateTime::QDateTime::currentDateTimeUtc().toTime_t();
        const auto lastSeen = affectLastSeen.value(CharacterAffectEnum::SNARED, 0);
        const bool noRecentSnareMessage = (now - lastSeen) > SNARED_MESSAGE_WINDOW;
        if (noRecentSnareMessage) {
            // Player is not snared after they moved and we did not get another snare message
            affects.remove(CharacterAffectEnum::SNARED);
            affectLastSeen.remove(CharacterAffectEnum::SNARED);
        }
    }

    group->getSelf()->setRoomId(roomId);

    issueLocalCharUpdate();
}

void Mmapper2Group::issueLocalCharUpdate()
{
    emit updateWidget();

    QMutexLocker locker(&networkLock);
    if (!group || getMode() == GroupManagerStateEnum::Off) {
        return;
    }

    if (network) {
        const QVariantMap &data = group->getSelf()->toVariantMap();
        emit sig_sendCharUpdate(data);
    }
}

void Mmapper2Group::relayMessageBox(const QString &message)
{
    emit log("GroupManager", message.toLatin1());
    emit messageBox("Group Manager", message);
}

void Mmapper2Group::gTellArrived(const QVariantMap &node)
{
    if (!node.contains("from") && node["from"].canConvert(QMetaType::QString)) {
        qWarning() << "From not found" << node;
        return;
    }
    const QString &from = node["from"].toString();

    if (!node.contains("text") && node["text"].canConvert(QMetaType::QString)) {
        qWarning() << "Text not found" << node;
        return;
    }
    const QString &text = node["text"].toString();

    auto color = getConfig().groupManager.groupTellColor;
    auto selection = getGroup()->selectByName(from.toLatin1());
    if (getConfig().groupManager.useGroupTellAnsi256Color && !selection->empty()) {
        auto character = selection->at(0);
        color = rgbToAnsi256String(character->getColor(), false);
    }
    emit log("GroupManager", QString("GTell from %1 arrived: %2").arg(from).arg(text));

    emit displayGroupTellEvent(color, from, text);
}

void Mmapper2Group::kickCharacter(const QByteArray &character)
{
    QMutexLocker locker(&networkLock);

    switch (getMode()) {
    case GroupManagerStateEnum::Off:
        throw std::runtime_error("network is down");
    case GroupManagerStateEnum::Client:
        throw std::invalid_argument("Only hosts can kick players");
    case GroupManagerStateEnum::Server:
        if (getGroup()->getSelf()->getName() == character)
            throw std::invalid_argument("You can't kick yourself");

        if (getGroup()->getCharByName(character) == nullptr)
            throw std::invalid_argument("Player does not exist");
        emit sig_kickCharacter(character);
        break;
    }
}

void Mmapper2Group::sendGroupTell(const QByteArray &tell)
{
    QMutexLocker locker(&networkLock);
    if (!network)
        throw std::runtime_error("network is down");

    emit sig_sendGroupTell(tell);
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

    const SharedGroupChar &self = getGroup()->getSelf();
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

    SharedGroupChar self = getGroup()->getSelf();
    QByteArray textHP;
    QByteArray textMana;
    QByteArray textMoves;
    CharacterAffects &affects = self->affects;

    static const QRegularExpression pRx(R"(^(?:\+ )?)"              //          Valar mudlle
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

    const bool wasRiding = affects.contains(CharacterAffectEnum::RIDING);
    const bool isRiding = match.captured(2).contains("R");
    if (!wasRiding && isRiding) {
        affects.insert(CharacterAffectEnum::RIDING);
        affectLastSeen.insert(CharacterAffectEnum::RIDING,
                              QDateTime::QDateTime::currentDateTimeUtc().toTime_t());

    } else if (wasRiding && !isRiding) {
        affects.remove(CharacterAffectEnum::RIDING);
    }

    const bool wasSearching = affects.contains(CharacterAffectEnum::SEARCH);
    if (wasSearching)
        affects.remove(CharacterAffectEnum::SEARCH);

    // REVISIT: Use remaining captures for more purposes and move this code to parser (?)
    textHP = match.captured(3).toLatin1();
    textMana = match.captured(4).toLatin1();
    textMoves = match.captured(5).toLatin1();
    bool inCombat = !match.captured(7).isEmpty();

    if (textHP == lastPrompt.textHP && textMana == lastPrompt.textMana
        && textMoves == lastPrompt.textMoves && inCombat == lastPrompt.inCombat
        && wasRiding == isRiding && !wasSearching) {
        return; // No update needed
    }

    if (textHP == "Dying") {
        // Incapacitated state overrides fighting state
        self->position = CharacterPositionEnum::INCAPACITATED;

    } else if (inCombat) {
        self->position = CharacterPositionEnum::FIGHTING;

    } else if (self->position != CharacterPositionEnum::DEAD) {
        // Recover standing state if we're done fighting (unless we died because we're dead)
        if (lastPrompt.inCombat) {
            self->position = CharacterPositionEnum::STANDING;
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
            if (current >= upper) \
                return upper; \
            else if (current <= lower) \
                return lower; \
            else \
                return current; \
        } \
    } while (false)

    // Estimate new numerical scores using prompt
    if (self->maxhp != 0) {
        // REVISIT: Replace this if/else tree with a data structure
        const auto calc_hp =
            [](const QByteArray &text, const double current, const double max) -> double {
            if (text.isEmpty() || text == "Healthy")
                return max;
            X_SCORE("Fine", max * 0.71, max * 0.99);
            X_SCORE("Hurt", max * 0.46, max * 0.70);
            X_SCORE("Wounded", max * 0.26, max * 0.45);
            X_SCORE("Bad", max * 0.11, max * 0.25);
            X_SCORE("Awful", max * 0.01, max * 0.10);
            return 0.0; // Dying
        };
        self->hp = static_cast<int>(calc_hp(textHP, self->hp, self->maxhp));
    }
    if (self->maxmana != 0) {
        const auto calc_mana =
            [](const QByteArray &text, const double current, const double max) -> double {
            if (text.isEmpty())
                return max;
            X_SCORE("Burning", max * 0.76, max * 0.99);
            X_SCORE("Hot", max * 0.46, max * 0.75);
            X_SCORE("Warm", max * 0.26, max * 0.45);
            X_SCORE("Cold", max * 0.11, max * 0.25);
            X_SCORE("Icy", max * 0.01, max * 0.10);
            return 0.0; // Frozen
        };
        self->mana = static_cast<int>(calc_mana(textMana, self->mana, self->maxmana));
    }
    if (self->maxmoves != 0) {
        const auto calc_moves = [](const QByteArray &text, const int current) -> int {
            if (text.isEmpty()) {
                if (current <= 50)
                    return 50;
                else
                    return current;
            }
            X_SCORE("Tired", 30, 49);
            X_SCORE("Slow", 15, 29);
            X_SCORE("Weak", 5, 14);
            X_SCORE("Fainting", 1, 4);
            return 0; // Exhausted
        };
        self->moves = calc_moves(textMoves, self->moves);
    }
#undef X_SCORE

    issueLocalCharUpdate();
}

void Mmapper2Group::updateCharacterPosition(const CharacterPositionEnum position)
{
    if (!group)
        return;

    // This naming convention for a reference is an anti-pattern;
    // the name implies it's a const value, but then we modify it.
    CharacterPositionEnum &oldPosition = group->getSelf()->position;

    if (oldPosition == position)
        return; // No update needed

    // Reset affects on death
    if (position == CharacterPositionEnum::DEAD)
        group->getSelf()->affects = CharacterAffects{};

    if (oldPosition == CharacterPositionEnum::DEAD && position != CharacterPositionEnum::STANDING)
        return; // Prefer dead state until we finish recovering some hp (i.e. stand)

    oldPosition = position;
    issueLocalCharUpdate();
}

void Mmapper2Group::updateCharacterAffect(const CharacterAffectEnum affect, const bool enable)
{
    if (!group)
        return;

    if (enable)
        affectLastSeen.insert(affect, QDateTime::QDateTime::currentDateTimeUtc().toTime_t());

    CharacterAffects &affects = getGroup()->getSelf()->affects;
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
    CharacterAffects &affects = getGroup()->getSelf()->affects;
    const int64_t now = QDateTime::QDateTime::currentDateTimeUtc().toTime_t();
    for (const CharacterAffectEnum affect : affectLastSeen.keys()) {
        const bool expired = [this, &affect, &now]() {
            const auto lastSeen = affectLastSeen.value(affect, 0);
            const auto timeout = s_affectTimeout.value(affect, DEFAULT_EXPIRE);
            const auto expiringAt = lastSeen + timeout;
            return expiringAt <= now;
        }();
        if (expired) {
            removedAtLeastOneAffect = true;
            affects.remove(affect);
            affectLastSeen.remove(affect);
        }
    }
    if (removedAtLeastOneAffect)
        issueLocalCharUpdate();
}

void Mmapper2Group::setPath(CommandQueue dirs)
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
    const SharedGroupChar &self = getGroup()->getSelf();
    self->hp = 0;
    self->maxhp = 0;
    self->mana = 0;
    self->maxmana = 0;
    self->moves = 0;
    self->maxmoves = 0;
    self->roomId = DEFAULT_ROOMID;
    self->position = CharacterPositionEnum::UNDEFINED;
    self->affects = CharacterAffects{};
    issueLocalCharUpdate();
}

void Mmapper2Group::sendLog(const QString &text)
{
    emit log("GroupManager", text);
}

GroupManagerStateEnum Mmapper2Group::getMode()
{
    QMutexLocker locker(&networkLock);
    return network ? network->getMode() : GroupManagerStateEnum::Off;
}

void Mmapper2Group::startNetwork()
{
    QMutexLocker locker(&networkLock);

    if (!network) {
        // Create network
        switch (getConfig().groupManager.state) {
        case GroupManagerStateEnum::Server:
            network = std::make_unique<GroupServer>(this);
            break;
        case GroupManagerStateEnum::Client:
            network = std::make_unique<GroupClient>(this);
            break;
        case GroupManagerStateEnum::Off:
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
                &CGroupCommunicator::sig_scheduleAction,
                getGroup(),
                &CGroup::slot_scheduleAction);
        connect(this,
                &Mmapper2Group::sig_kickCharacter,
                network.get(),
                &CGroupCommunicator::kickCharacter);
        connect(this,
                &Mmapper2Group::sig_sendGroupTell,
                network.get(),
                &CGroupCommunicator::sendGroupTell);
        connect(this,
                &Mmapper2Group::sig_sendCharUpdate,
                network.get(),
                QOverload<const QVariantMap &>::of(&CGroupCommunicator::sendCharUpdate));
        connect(this,
                &Mmapper2Group::sig_sendSelfRename,
                network.get(),
                &CGroupCommunicator::sendSelfRename);
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

void Mmapper2Group::setMode(const GroupManagerStateEnum newMode)
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
