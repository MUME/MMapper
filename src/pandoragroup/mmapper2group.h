#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <memory>
#include <QArgument>
#include <QMap>
#include <QMutex>
#include <QObject>
#include <QTimer>
#include <QVariantMap>

#include "../global/WeakHandle.h"
#include "../global/roomid.h"
#include "GroupManagerApi.h"
#include "mmapper2character.h"

class GroupAuthority;
class CGroupCommunicator;
class CGroup;
class Mmapper2Group;
class CommandQueue;
enum class GroupManagerStateEnum { Off = 0, Client = 1, Server = 2 };

class Mmapper2Group final : public QObject
{
public:
    Q_OBJECT

signals:
    // MainWindow::log (via MainWindow)
    void log(const QString &, const QString &);
    // MainWindow::groupNetworkStatus (via MainWindow)
    void networkStatus(bool);
    // MapCanvas::requestUpdate (via MainWindow)
    void updateMapCanvas(); // redraw the opengl screen

    // sent to ParserXML::sendGTellToUser (via Proxy)
    void displayGroupTellEvent(const QByteArray &tell); // displays gtell from remote user

    // GroupWidget::messageBox (via GroupWidget)
    void messageBox(QString title, QString message);
    // GroupWidget::updateLabels (via GroupWidget)
    void updateWidget(); // update group widget

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
    explicit Mmapper2Group(QObject *parent = nullptr);
    ~Mmapper2Group() override;

    void start();
    void stop();

    GroupManagerStateEnum getMode();

    GroupAuthority *getAuthority() { return authority.get(); }
    CGroup *getGroup() { return group.get(); }

public:
    GroupManagerApi &getGroupManagerApi() { return m_groupManagerApi; }

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
    void setCharacterRoomId(RoomId pos);
    void setMode(GroupManagerStateEnum newState);
    void startNetwork();
    void stopNetwork();
    void updateSelf(); // changing settings

    void setPath(CommandQueue);
    void reset();

protected slots:
    // Communicator
    void gTellArrived(const QVariantMap &node);
    void relayMessageBox(const QString &message);
    void sendLog(const QString &);
    void characterChanged(bool updateCanvas);
    void onAffectTimeout();
    void slot_stopInternal();

private:
    struct
    {
        QByteArray textHP;
        QByteArray textMoves;
        QByteArray textMana;
        bool inCombat = false;
    } lastPrompt;

    std::atomic_int m_calledStopInternal{0};
    QTimer affectTimer;
    QMap<CharacterAffectEnum, int64_t> affectLastSeen;
    using AffectTimeout = QMap<CharacterAffectEnum, int32_t>;
    static const AffectTimeout s_affectTimeout;

    bool init();
    void issueLocalCharUpdate();

    QMutex networkLock;
    std::unique_ptr<QThread> thread;
    std::unique_ptr<GroupAuthority> authority;
    std::unique_ptr<CGroupCommunicator> network;
    std::unique_ptr<CGroup> group;
};
