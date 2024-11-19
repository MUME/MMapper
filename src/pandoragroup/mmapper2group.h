#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/ConfigEnums.h"
#include "../global/WeakHandle.h"
#include "../global/roomid.h"
#include "GroupManagerApi.h"
#include "mmapper2character.h"

#include <memory>

#include <QArgument>
#include <QMap>
#include <QMutex>
#include <QObject>
#include <QTimer>
#include <QVariantMap>

class GmcpMessage;
class GroupAuthority;
class CGroupCommunicator;
class CGroup;
class Mmapper2Group;
class CommandQueue;

class Mmapper2Group final : public QObject
{
public:
    Q_OBJECT

private:
    void log(const QString &msg) { emit sig_log("GroupManager", msg); }
    void messageBox(const QString &msg) { emit sig_messageBox("GroupManager", msg); }

signals:
    // MainWindow::log (via MainWindow)
    void sig_log(const QString &, const QString &);
    // MainWindow::groupNetworkStatus (via MainWindow)
    void sig_networkStatus(bool);
    // MapCanvas::requestUpdate (via MainWindow)
    void sig_updateMapCanvas(); // redraw the opengl screen

    // sent to ParserXML::sendGTellToUser (via Proxy)
    void sig_displayGroupTellEvent(const QString &color,
                                   const QString &name,
                                   const QString &message);

    // GroupWidget::messageBox (via GroupWidget)
    void sig_messageBox(QString title, QString message);
    // GroupWidget::updateLabels (via GroupWidget)
    void sig_updateWidget(); // update group widget

    // Mmapper2Group::slot_stopInternal
    void sig_invokeStopInternal();

    // CGroupCommunicator::sendGroupTell
    void sig_sendGroupTell(const QByteArray &tell);
    // CGroupCommunicator::kickCharacter
    void sig_kickCharacter(const QByteArray &character);
    // CGroupCommunicator::sendCharUpdate
    void sig_sendCharUpdate(const QVariantMap &map);
    // CGroupCommunicator::sendSelfRename
    void sig_sendSelfRename(const QByteArray &, const QByteArray &);

public:
    explicit Mmapper2Group(QObject *parent);
    ~Mmapper2Group() final;

    void start();
    void stop();

    NODISCARD GroupManagerStateEnum getMode();

    NODISCARD GroupAuthority *getAuthority() { return authority.get(); }
    NODISCARD CGroup *getGroup() { return group.get(); }

public:
    NODISCARD GroupManagerApi &getGroupManagerApi() { return m_groupManagerApi; }

private:
    WeakHandleLifetime<Mmapper2Group> m_weakHandleLifetime{*this};
    GroupManagerApi m_groupManagerApi{m_weakHandleLifetime.getWeakHandle()};
    friend GroupManagerApi;

protected:
    void sendGroupTell(const QByteArray &tell); // sends gtell from local user
    void kickCharacter(const QByteArray &character);
    void parseScoreInformation(const QByteArray &score);
    void parsePromptInformation(const QByteArray &prompt);
    void updateCharacterPosition(CharacterPositionEnum);
    void updateCharacterAffect(CharacterAffectEnum, bool);

public slots:
    void slot_setCharacterRoomId(RoomId pos);
    void slot_setMode(GroupManagerStateEnum newState);
    void slot_startNetwork();
    void slot_stopNetwork();
    void slot_updateSelf(); // changing settings

    void slot_setPath(CommandQueue);
    void slot_reset();
    void slot_parseGmcpInput(const GmcpMessage &msg);

protected slots:
    // Communicator
    void slot_gTellArrived(const QVariantMap &node);
    void slot_relayMessageBox(const QString &message);
    void slot_sendLog(const QString &);
    void slot_characterChanged(bool updateCanvas);
    void slot_onAffectTimeout();
    void slot_stopInternal();

private:
    struct NODISCARD LastPrompt final
    {
        QByteArray textHP;
        QByteArray textMoves;
        QByteArray textMana;

        void reset() { *this = LastPrompt{}; }
    };
    LastPrompt lastPrompt;

    std::atomic_int m_calledStopInternal{0};
    QTimer affectTimer;
    QMap<CharacterAffectEnum, int64_t> affectLastSeen;
    using AffectTimeout = QMap<CharacterAffectEnum, int32_t>;
    static const AffectTimeout s_affectTimeout;

    bool init();
    void issueLocalCharUpdate();
    bool setCharacterPosition(CharacterPositionEnum pos);
    bool setCharacterScore(int hp, int maxhp, int mana, int maxmana, int mp, int maxmp);
    void renameCharacter(QByteArray newname);

    QRecursiveMutex networkLock;
    std::unique_ptr<QThread> thread;
    std::unique_ptr<GroupAuthority> authority;
    std::unique_ptr<CGroupCommunicator> network;
    std::unique_ptr<CGroup> group;
};
