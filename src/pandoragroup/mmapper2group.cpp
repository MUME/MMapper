// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "mmapper2group.h"

#include "../configuration/configuration.h"
#include "../global/AnsiTextUtils.h"
#include "../global/MakeQPointer.h"
#include "../global/PrintUtils.h"
#include "../parser/CommandQueue.h"
#include "../proxy/GmcpMessage.h"
#include "CGroup.h"
#include "CGroupChar.h"
#include "GroupClient.h"
#include "GroupServer.h"
#include "groupauthority.h"
#include "groupselection.h"
#include "mmapper2character.h"

#include <map>
#include <memory>

#include <QColor>
#include <QDateTime>
#include <QMessageLogContext>
#include <QMutex>
#include <QThread>
#include <QVariantMap>
#include <QtCore>

using Seconds = std::chrono::seconds;
using Minutes = std::chrono::minutes;
struct NODISCARD AffectTimeoutMap final : private std::map<CharacterAffectEnum, Seconds>
{
    using map::map;
    NODISCARD Seconds value(const CharacterAffectEnum key, const Seconds defaultValue) const
    {
        if (const auto it = find(key); it != end()) {
            return it->second;
        }
        return defaultValue;
    }
};

static constexpr const auto TWO_MINUTES = Minutes{2};
static constexpr const auto THIRTY_MINUTES = Minutes{30};
static constexpr const auto DEFAULT_EXPIRE = THIRTY_MINUTES;

static const AffectTimeoutMap g_affectTimeoutMap = {{CharacterAffectEnum::BASHED, Seconds{4}},
                                                    {CharacterAffectEnum::BLIND, THIRTY_MINUTES},
                                                    {CharacterAffectEnum::POISONED, Minutes{5}},
                                                    {CharacterAffectEnum::SLEPT, THIRTY_MINUTES},
                                                    {CharacterAffectEnum::BLEEDING, TWO_MINUTES},
                                                    {CharacterAffectEnum::HUNGRY, TWO_MINUTES},
                                                    {CharacterAffectEnum::THIRSTY, TWO_MINUTES}};

NODISCARD static CharacterPositionEnum toCharacterPosition(const QString &str)
{
#define CASE2(UPPER_CASE, lower_case, CamelCase, friendly) \
    do { \
        if (str == #lower_case) { \
            return CharacterPositionEnum::UPPER_CASE; \
        } \
    } while (false);
    X_FOREACH_CHARACTER_POSITION(CASE2)
#undef CASE2
    return CharacterPositionEnum::UNDEFINED;
}

Mmapper2Group::Mmapper2Group(QObject *const parent)
    : QObject{parent}
    , m_affectTimer{this}
{
    auto &affectTimer = m_affectTimer;
    affectTimer.setInterval(1000);
    affectTimer.setSingleShot(false);
    affectTimer.start();
    connect(&affectTimer, &QTimer::timeout, this, &Mmapper2Group::slot_onAffectTimeout);
}

Mmapper2Group::~Mmapper2Group()
{
    // Stop the network
    stop();

    qInfo() << "Terminated Group Manager service";
}

void Mmapper2Group::start()
{
    init();
}

void Mmapper2Group::init()
{
    m_group = std::make_unique<CGroup>(this);
    m_authority = std::make_unique<GroupAuthority>(this);

    connect(m_group.get(), &CGroup::sig_log, this, &Mmapper2Group::slot_sendLog);
    connect(m_group.get(),
            &CGroup::sig_characterChanged,
            this,
            &Mmapper2Group::slot_characterChanged,
            Qt::QueuedConnection);

    emit sig_updateWidget();
}

void Mmapper2Group::stop()
{
    assert(QThread::currentThread() == QObject::thread());
    m_affectTimer.stop();
    slot_stopNetwork();
    ++m_calledStopInternal;
}

void Mmapper2Group::slot_characterChanged(bool updateCanvas)
{
    emit sig_updateWidget();
    if (updateCanvas) {
        emit sig_updateMapCanvas();
    }
}

void Mmapper2Group::slot_updateSelf()
{
    QMutexLocker locker(&m_networkLock);
    if (m_group == nullptr) {
        return;
    }

    CGroupChar &self = deref(m_group->getSelf());
    const Configuration::GroupManagerSettings &conf = getConfig().groupManager;

    // Older code assumed that only one change can occur at a time,
    // but we now support the possibility of multiple changes at once.
    bool changed = false;

    if (self.getLabel() != conf.charName) {
        self.setLabel(conf.charName);
        changed = true;
    }

    if (self.getColor() != conf.color) {
        self.setColor(conf.color);
        changed = true;
    }

    if (changed) {
        issueLocalCharUpdate();
    }
}

void Mmapper2Group::slot_setCharacterRoomId(RoomId roomId)
{
    QMutexLocker locker(&m_networkLock);
    if (m_group == nullptr) {
        return;
    }

    CGroupChar &self = deref(m_group->getSelf());
    if (self.getRoomId() == roomId) {
        return; // No update needed
    }

    // Check if we are still snared
    static const constexpr auto SNARED_MESSAGE_WINDOW = 1;
    CharacterAffectFlags &affects = self.affects;
    if (affects.contains(CharacterAffectEnum::SNARED)) {
        const int64_t now = QDateTime::QDateTime::currentDateTimeUtc().toSecsSinceEpoch();
        const auto lastSeen = m_affectLastSeen.value(CharacterAffectEnum::SNARED, 0);
        const bool noRecentSnareMessage = (now - lastSeen) > SNARED_MESSAGE_WINDOW;
        if (noRecentSnareMessage) {
            // Player is not snared after they moved and we did not get another snare message
            affects.remove(CharacterAffectEnum::SNARED);
            m_affectLastSeen.remove(CharacterAffectEnum::SNARED);
        }
    }

    self.setRoomId(roomId);

    issueLocalCharUpdate();
}

void Mmapper2Group::issueLocalCharUpdate()
{
    emit sig_updateWidget();

    QMutexLocker locker(&m_networkLock);
    if (m_group == nullptr || getMode() == GroupManagerStateEnum::Off) {
        return;
    }

    if (m_network != nullptr) {
        const QVariantMap &data = m_group->getSelf()->toVariantMap();
        emit sig_sendCharUpdate(data);
    }
}

void Mmapper2Group::slot_relayMessageBox(const QString &message)
{
    log(message.toLatin1());
    messageBox(message);
}

void Mmapper2Group::slot_gTellArrived(const QVariantMap &node)
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

    auto name = from;
    auto color = getConfig().groupManager.groupTellColor;
    auto selection = getGroup()->selectByName(from.toLatin1());
    if (!selection->empty()) {
        const auto &character = selection->at(0);
        if (!character->getLabel().isEmpty() && character->getLabel() != character->getName()) {
            name = QString("%1 (%2)").arg(QString::fromLatin1(character->getName()),
                                          QString::fromLatin1(character->getLabel()));
        }
        if (getConfig().groupManager.useGroupTellAnsi256Color) {
            color = mmqt::rgbToAnsi256String(character->getColor(),
                                             AnsiColor16LocationEnum::Background);
        }
    }
    log(QString("GTell from %1 arrived: %2").arg(from, text));

    emit sig_displayGroupTellEvent(color, name, text);
}

void Mmapper2Group::kickCharacter(const QByteArray &character)
{
    QMutexLocker locker(&m_networkLock);

    switch (getMode()) {
    case GroupManagerStateEnum::Off:
        throw std::runtime_error("network is down");
    case GroupManagerStateEnum::Client:
        throw std::invalid_argument("Only hosts can kick players");
    case GroupManagerStateEnum::Server:
        if (getGroup()->getSelf()->getName() == character) {
            throw std::invalid_argument("You can't kick yourself");
        }
        if (getGroup()->getCharByName(character) == nullptr) {
            throw std::invalid_argument("Player does not exist");
        }
        emit sig_kickCharacter(character);
        break;
    }
}

void Mmapper2Group::sendGroupTell(const QByteArray &tell)
{
    QMutexLocker locker(&m_networkLock);
    if (m_network == nullptr) {
        throw std::runtime_error("network is down");
    }

    emit sig_sendGroupTell(tell);
}

void Mmapper2Group::parseScoreInformation(const QByteArray &score)
{
    if (m_group == nullptr) {
        return;
    }

    static const QRegularExpression sRx(R"(^(?:You (?:have|report) )?)" // 'info' support
                                        R"((\d+)\/(\d+) hits?)"         // Group 1/2 hits
                                        R"(,?(?: (\d+)\/(\d+) mana,)?)" // Group 3/4 mana
                                        R"( and (\d+)\/(\d+) move)"     // Group 5/6 moves
                                        R"((?:ment point)?s.)");
    QRegularExpressionMatch match = sRx.match(score);
    if (!match.hasMatch()) {
        return;
    }
    const int hp = match.captured(1).toInt();
    const int maxhp = match.captured(2).toInt();
    const int mana = match.captured(3).toInt();
    const int maxmana = match.captured(4).toInt();
    const int moves = match.captured(5).toInt();
    const int maxmoves = match.captured(6).toInt();

    if (setCharacterScore(hp, maxhp, mana, maxmana, moves, maxmoves)) {
        issueLocalCharUpdate();
    }
}

bool Mmapper2Group::setCharacterScore(const int hp,
                                      const int maxhp,
                                      const int mana,
                                      const int maxmana,
                                      const int moves,
                                      const int maxmoves)
{
    CGroupChar &self = deref(getGroup()->getSelf());
    if (self.hp == hp && self.maxhp == maxhp && self.mana == mana && self.maxmana == maxmana
        && self.moves == moves && self.maxmoves == maxmoves)
        return false; // No update needed

    log(QString("Updated score: %1/%2 hits, %3/%4 mana, and %5/%6 moves.")
            .arg(hp)
            .arg(maxhp)
            .arg(mana)
            .arg(maxmana)
            .arg(moves)
            .arg(maxmoves));

    self.setScore(hp, maxhp, mana, maxmana, moves, maxmoves);
    return true;
}

void Mmapper2Group::parsePromptInformation(const QByteArray &prompt)
{
    if (m_group == nullptr) {
        return;
    }

    static const QRegularExpression pRx(R"((?: HP:([^ >]+))?)"     // Group 1: HP
                                        R"((?: Mana:([^ >]+))?)"   // Group 2: Mana
                                        R"((?: Move:([^ >]+))?)"); // Group 3: Move
    QRegularExpressionMatch match = pRx.match(prompt);
    if (!match.hasMatch()) {
        return;
    }

    CGroupChar &self = deref(getGroup()->getSelf());
    QByteArray textHP;
    QByteArray textMana;
    QByteArray textMoves;
    CharacterAffectFlags &affects = self.affects;

    const bool wasSearching = affects.contains(CharacterAffectEnum::SEARCH);
    if (wasSearching) {
        affects.remove(CharacterAffectEnum::SEARCH);
    }

    // REVISIT: Use remaining captures for more purposes and move this code to parser (?)
    textHP = mmqt::toQByteArrayLatin1(match.captured(1));
    textMana = mmqt::toQByteArrayLatin1(match.captured(2));
    textMoves = mmqt::toQByteArrayLatin1(match.captured(3));

    auto &lastPrompt = m_lastPrompt;
    if (textHP == lastPrompt.textHP && textMana == lastPrompt.textMana
        && textMoves == lastPrompt.textMoves && !wasSearching) {
        return; // No update needed
    }

    // Update last prompt values
    lastPrompt.textHP = textHP;
    lastPrompt.textMana = textMana;
    lastPrompt.textMoves = textMoves;

#define X_SCORE(target, lower, upper) \
    do { \
        if (text == (target)) { \
            if (current >= (upper)) { \
                return upper; \
            } else if (current <= (lower)) { \
                return lower; \
            } else { \
                return current; \
            } \
        } \
    } while (false)

    // Estimate new numerical scores using prompt
    if (self.maxhp != 0) {
        // REVISIT: Replace this if/else tree with a data structure
        const auto calc_hp =
            [](const QByteArray &text, const double current, const double max) -> double {
            if (text.isEmpty() || text == "Healthy") {
                return max;
            }
            X_SCORE("Fine", max * 0.71, max * 0.99);
            X_SCORE("Hurt", max * 0.46, max * 0.70);
            X_SCORE("Wounded", max * 0.26, max * 0.45);
            X_SCORE("Bad", max * 0.11, max * 0.25);
            X_SCORE("Awful", max * 0.01, max * 0.10);
            return 0.0; // Dying
        };
        self.hp = static_cast<int>(calc_hp(textHP, self.hp, self.maxhp));
    }
    if (self.maxmana != 0) {
        const auto calc_mana =
            [](const QByteArray &text, const double current, const double max) -> double {
            if (text.isEmpty()) {
                return max;
            }
            X_SCORE("Burning", max * 0.76, max * 0.99);
            X_SCORE("Hot", max * 0.46, max * 0.75);
            X_SCORE("Warm", max * 0.26, max * 0.45);
            X_SCORE("Cold", max * 0.11, max * 0.25);
            X_SCORE("Icy", max * 0.01, max * 0.10);
            return 0.0; // Frozen
        };
        self.mana = static_cast<int>(calc_mana(textMana, self.mana, self.maxmana));
    }
    if (self.maxmoves != 0) {
        const auto calc_moves = [](const QByteArray &text, const int current) -> int {
            if (text.isEmpty()) {
                return std::max(50, current);
            }
            X_SCORE("Tired", 30, 49);
            X_SCORE("Slow", 15, 29);
            X_SCORE("Weak", 5, 14);
            X_SCORE("Fainting", 1, 4);
            return 0; // Exhausted
        };
        self.moves = calc_moves(textMoves, self.moves);
    }
#undef X_SCORE

    issueLocalCharUpdate();
}

bool Mmapper2Group::setCharacterPosition(const CharacterPositionEnum position)
{
    CGroupChar &self = deref(getGroup()->getSelf());
    const CharacterPositionEnum oldPosition = self.position;

    if (oldPosition == position) {
        return false; // No update needed
    }

    // Reset affects on death
    if (position == CharacterPositionEnum::DEAD) {
        self.affects = CharacterAffectFlags{};
    }

    if (oldPosition == CharacterPositionEnum::DEAD && position != CharacterPositionEnum::STANDING) {
        return false; // Prefer dead state until we finish recovering some hp (i.e. stand)
    }

    self.position = position;
    return true;
}

void Mmapper2Group::updateCharacterPosition(const CharacterPositionEnum position)
{
    if (m_group == nullptr) {
        return;
    }

    if (setCharacterPosition(position)) {
        issueLocalCharUpdate();
    }
}

void Mmapper2Group::updateCharacterAffect(const CharacterAffectEnum affect, const bool enable)
{
    if (m_group == nullptr) {
        return;
    }

    if (enable) {
        m_affectLastSeen.insert(affect,
                                QDateTime::QDateTime::currentDateTimeUtc().toSecsSinceEpoch());
    }

    CharacterAffectFlags &affects = getGroup()->getSelf()->affects;
    if (enable == affects.contains(affect)) {
        return; // No update needed
    }

    if (enable) {
        affects.insert(affect);
    } else {
        affects.remove(affect);
        m_affectLastSeen.remove(affect);
    }

    issueLocalCharUpdate();
}

void Mmapper2Group::slot_onAffectTimeout()
{
    auto &affectLastSeen = m_affectLastSeen;
    if (affectLastSeen.isEmpty()) {
        return;
    }

    bool removedAtLeastOneAffect = false;
    CharacterAffectFlags &affects = getGroup()->getSelf()->affects;
    const int64_t now = QDateTime::QDateTime::currentDateTimeUtc().toSecsSinceEpoch();
    for (const CharacterAffectEnum affect : affectLastSeen.keys()) {
        const bool expired = [&affect, &affectLastSeen, &now]() {
            const auto lastSeen = affectLastSeen.value(affect, 0);
            const auto timeout = g_affectTimeoutMap.value(affect, DEFAULT_EXPIRE);
            const auto expiringAt = lastSeen + static_cast<Seconds>(timeout).count();
            return expiringAt <= now;
        }();
        if (expired) {
            removedAtLeastOneAffect = true;
            affects.remove(affect);
            affectLastSeen.remove(affect);
        }
    }
    if (removedAtLeastOneAffect) {
        issueLocalCharUpdate();
    }
}

void Mmapper2Group::slot_setPath(CommandQueue dirs)
{
    if (m_group != nullptr) {
        if (const auto &self = m_group->getSelf()) {
            self->prespam = std::move(dirs);
        }
    }
}

void Mmapper2Group::slot_reset()
{
    QMutexLocker locker(&m_networkLock);

    // Reset prompt
    m_lastPrompt.reset();

    // Reset character
    auto &self = deref(getGroup()->getSelf());
    self.reset();

    // Reset name to label
    renameCharacter(getConfig().groupManager.charName);

    issueLocalCharUpdate();
}

void Mmapper2Group::slot_parseGmcpInput(const GmcpMessage &msg)
{
    if (m_group == nullptr) {
        return;
    }

    if (msg.isCharVitals() && msg.getJsonDocument().has_value()
        && msg.getJsonDocument()->isObject()) {
        // "Char.Vitals {\"hp\":100,\"maxhp\":100,\"mana\":100,\"maxmana\":100,\"mp\":139,\"maxmp\":139}"
        const GmcpJsonDocument &doc = msg.getJsonDocument().value();
        const auto &obj = doc.object();

        CGroupChar &self = deref(getGroup()->getSelf());
        CharacterAffectFlags &affects = self.affects;

        bool update = false;

        if (obj.contains("hp") || obj.contains("maxhp") || obj.contains("mana")
            || obj.contains("maxmana") || obj.contains("mp") || obj.contains("maxmp")) {
            const int hp = obj.value("hp").toInt(self.hp);
            const int maxhp = obj.value("maxhp").toInt(self.maxhp);
            const int mana = obj.value("mana").toInt(self.mana);
            const int maxmana = obj.value("maxmana").toInt(self.maxmana);
            const int mp = obj.value("mp").toInt(self.moves);
            const int maxmp = obj.value("maxmp").toInt(self.maxmoves);
            if (setCharacterScore(hp, maxhp, mana, maxmana, mp, maxmp)) {
                update = true;
            }
        }

        if (obj.contains("ride") && obj.value("ride").isBool()) {
            const bool wasRiding = affects.contains(CharacterAffectEnum::RIDING);
            const bool isRiding = obj.value("ride").toBool();
            if (isRiding) {
                affects.insert(CharacterAffectEnum::RIDING);
                m_affectLastSeen
                    .insert(CharacterAffectEnum::RIDING,
                            QDateTime::QDateTime::currentDateTimeUtc().toSecsSinceEpoch());
            } else {
                affects.remove(CharacterAffectEnum::RIDING);
            }
            if (isRiding != wasRiding) {
                update = true;
            }
        }

        if (obj.contains("position") && obj.value("position").isString()) {
            const auto position = toCharacterPosition(obj.value("position").toString());
            if (setCharacterPosition(position)) {
                update = true;
            }
        }

        if (update) {
            issueLocalCharUpdate();
        }

        return;
    }

    if (msg.isCharName() && msg.getJsonDocument().has_value() && msg.getJsonDocument()->isObject()) {
        // "Char.Name" "{\"fullname\":\"Gandalf the Grey\",\"name\":\"Gandalf\"}"
        const QJsonDocument &doc = msg.getJsonDocument().value();
        const auto &obj = doc.object();
        const auto &name = obj.value("name");
        if (!name.isString()) {
            return;
        }

        renameCharacter(name.toString().toLatin1());
        issueLocalCharUpdate();
        return;
    }
}

void Mmapper2Group::renameCharacter(QByteArray newname)
{
    const auto &oldname = getGroup()->getSelf()->getName();
    if (getGroup()->isNamePresent(newname)) {
        const auto &fallback = getConfig().groupManager.charName;
        if (getGroup()->isNamePresent(fallback))
            newname = oldname;
        else
            newname = fallback;
    }
    if (oldname != newname) {
        QMutexLocker locker(&m_networkLock);

        // Inform the server that we're renaming ourselves
        if (m_network != nullptr) {
            // why does this check network? signal should handle.
            emit sig_sendSelfRename(oldname, newname);
        }
        m_group->getSelf()->setName(newname);
    }
}

void Mmapper2Group::slot_sendLog(const QString &text)
{
    log(text);
}

GroupManagerStateEnum Mmapper2Group::getMode()
{
    QMutexLocker locker(&m_networkLock);
    return m_network != nullptr ? m_network->getMode() : GroupManagerStateEnum::Off;
}

void Mmapper2Group::slot_startNetwork()
{
    QMutexLocker locker(&m_networkLock);

    auto &network = m_network;
    if (network == nullptr) {
        // Create network
        switch (Mmapper2Group::getConfigState()) {
        case GroupManagerStateEnum::Server:
            network = mmqt::makeQPointer<GroupServer>(this);
            break;
        case GroupManagerStateEnum::Client:
            network = mmqt::makeQPointer<GroupClient>(this);
            break;
        case GroupManagerStateEnum::Off:
            // NOTE: network isn't created in this case.
            return;
        }

        connect(network, &CGroupCommunicator::sig_sendLog, this, &Mmapper2Group::slot_sendLog);
        connect(network,
                &CGroupCommunicator::sig_messageBox,
                this,
                &Mmapper2Group::slot_relayMessageBox);
        connect(network,
                &CGroupCommunicator::sig_gTellArrived,
                this,
                &Mmapper2Group::slot_gTellArrived);
        connect(network, &CGroupCommunicator::destroyed, this, [this]() {
            emit sig_networkStatus(false);
        });
        connect(network,
                &CGroupCommunicator::sig_scheduleAction,
                getGroup(),
                &CGroup::slot_scheduleAction);
        connect(this,
                &Mmapper2Group::sig_kickCharacter,
                network,
                &CGroupCommunicator::slot_kickCharacter);
        connect(this,
                &Mmapper2Group::sig_sendGroupTell,
                network,
                &CGroupCommunicator::slot_sendGroupTell);
        connect(this,
                &Mmapper2Group::sig_sendCharUpdate,
                network,
                QOverload<const QVariantMap &>::of(&CGroupCommunicator::slot_sendCharUpdate));
        connect(this,
                &Mmapper2Group::sig_sendSelfRename,
                network,
                &CGroupCommunicator::slot_sendSelfRename);
    }

    std::ignore = deref(network);

    // REVISIT: What about if the network is already started?
    if (network->start()) {
        emit sig_networkStatus(true);
        emit sig_updateWidget();

        if (getConfig().groupManager.rulesWarning) {
            emit sig_messageBox("Warning: MUME Rules",
                                "Please read and comply with RULES INTERRACE"
                                " and RULES ACTIONS to use Group Manager.");
        }
        qDebug() << "Network up";
    } else {
        network->stop();
        qDebug() << "Network failed to start";
    }
}

void Mmapper2Group::slot_stopNetwork()
{
    QMutexLocker locker(&m_networkLock);
    if (m_network != nullptr) {
        m_network->stop();
        qDebug() << "Network down";
    }
}

void Mmapper2Group::slot_setMode(const GroupManagerStateEnum newMode)
{
    QMutexLocker locker(&m_networkLock);

    Mmapper2Group::setConfigState(newMode); // Ensure config matches reality

    const auto currentState = getMode();
    if (currentState == newMode) {
        return; // Do not bother changing states if we're already in it
    }

    // Stop previous network if it changed
    slot_stopNetwork();

    qDebug() << "Network type set to" << static_cast<int>(newMode);
}

GroupManagerStateEnum Mmapper2Group::getConfigState()
{
    return getConfig().groupManager.state;
}

void Mmapper2Group::setConfigState(GroupManagerStateEnum state)
{
    setConfig().groupManager.state = state;
}
