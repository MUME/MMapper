#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/ConfigEnums.h"
#include "../global/WeakHandle.h"
#include "../map/Map.h"
#include "../map/roomid.h"
#include "GroupManagerApi.h"
#include "mmapper2character.h"

#include <memory>

#include <QArgument>
#include <QMap>
#include <QObject>
#include <QTimer>
#include <QVariantMap>

class CGroup;
class CGroupCommunicator;
class CommandQueue;
class GmcpMessage;
class GroupAuthority;
class Mmapper2Group;

class NODISCARD_QOBJECT Mmapper2Group final : public QObject
{
    Q_OBJECT

private:
    struct NODISCARD LastPrompt final
    {
        QString textHP;
        QString textMoves;
        QString textMana;

        void reset() { *this = LastPrompt{}; }
    };
    LastPrompt m_lastPrompt;

    std::atomic_int m_calledStopInternal{0};
    QTimer m_affectTimer;
    QMap<CharacterAffectEnum, int64_t> m_affectLastSeen;

    std::unique_ptr<GroupAuthority> m_authority;
    QPointer<CGroupCommunicator> m_network;
    std::unique_ptr<CGroup> m_group;

public:
    NODISCARD static GroupManagerStateEnum getConfigState();
    static void setConfigState(GroupManagerStateEnum state);

private:
    void log(const QString &msg) { emit sig_log("GroupManager", msg); }
    void messageBox(const QString &msg) { emit sig_messageBox("GroupManager", msg); }

public:
    explicit Mmapper2Group(QObject *parent);
    ~Mmapper2Group() final;

    void start();
    void stop();

    NODISCARD GroupManagerStateEnum getMode();

    NODISCARD GroupAuthority *getAuthority() { return m_authority.get(); }
    NODISCARD CGroup *getGroup() { return m_group.get(); }

public:
    NODISCARD GroupManagerApi &getGroupManagerApi() { return m_groupManagerApi; }

private:
    WeakHandleLifetime<Mmapper2Group> m_weakHandleLifetime{*this};
    GroupManagerApi m_groupManagerApi{m_weakHandleLifetime.getWeakHandle()};
    friend GroupManagerApi;

protected:
    void sendGroupTell(const QString &tell); // sends gtell from local user
    void kickCharacter(const QString &character);
    void parseScoreInformation(const QString &score);
    void parsePromptInformation(const QString &prompt);
    void updateCharacterPosition(CharacterPositionEnum);
    void updateCharacterAffect(CharacterAffectEnum, bool);

private:
    void setCharRoomId(ServerRoomId srvId, ExternalRoomId extId);

private:
    void init();
    void issueLocalCharUpdate();
    NODISCARD bool setCharacterPosition(CharacterPositionEnum pos);
    NODISCARD bool setCharacterScore(int hp, int maxhp, int mana, int maxmana, int mp, int maxmp);
    void renameCharacter(QByteArray newname) = delete;
    void renameCharacter(QString newname);

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

    // CGroupCommunicator::sendGroupTell
    void sig_sendGroupTell(const QString &tell);
    // CGroupCommunicator::kickCharacter
    void sig_kickCharacter(const QString &character);
    // CGroupCommunicator::sendCharUpdate
    void sig_sendCharUpdate(const QVariantMap &map);
    // CGroupCommunicator::sendSelfRename
    void sig_sendSelfRename(const QString &, const QString &);

public slots:
    void slot_setCharRoomIdFromServer(ServerRoomId srvId, ExternalRoomId extId)
    {
        setCharRoomId(srvId, extId);
    }
    void slot_setCharRoomIdEstimated(ServerRoomId srvId, ExternalRoomId extId)
    {
        setCharRoomId(srvId, extId);
    }
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
};
