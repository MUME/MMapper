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

#include "../global/roomid.h"
#include "mmapper2character.h"

class GroupAuthority;
class CGroupCommunicator;
class CGroup;
class Mmapper2Group;
class CommandQueue;
enum class GroupManagerState { Off = 0, Client = 1, Server = 2 };

class Mmapper2Group final : public QObject
{
public:
    Q_OBJECT

signals:
    void log(const QString &, const QString &);
    void displayGroupTellEvent(const QByteArray &tell); // displays gtell from remote user
    void messageBox(QString title, QString message);
    void networkStatus(bool);
    void updateWidget();    // update group widget
    void updateMapCanvas(); // redraw the opengl screen
    void sig_invokeStopInternal();

public:
    explicit Mmapper2Group(QObject *parent = nullptr);
    virtual ~Mmapper2Group();

    void start();
    void stop();

    GroupManagerState getMode();

    GroupAuthority *getAuthority() { return authority.get(); }
    CGroup *getGroup() { return group.get(); }

public slots:
    void setCharacterRoomId(RoomId pos);
    void setMode(GroupManagerState newState);
    void startNetwork();
    void stopNetwork();
    void updateSelf(); // changing settings

    void sendGroupTell(const QByteArray &tell); // sends gtell from local user
    void kickCharacter(const QByteArray &character);
    void parseScoreInformation(const QByteArray &score);
    void parsePromptInformation(const QByteArray &prompt);
    void updateCharacterPosition(CharacterPosition);
    void updateCharacterAffect(CharacterAffect, bool);
    void setPath(CommandQueue, bool);
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
        QByteArray textHP{};
        QByteArray textMoves{};
        QByteArray textMana{};
        bool inCombat{false};
    } lastPrompt;

    std::atomic_int m_calledStopInternal{0};
    QTimer affectTimer;
    QMap<CharacterAffect, uint64_t> affectLastSeen;
    using AffectTimeout = QMap<CharacterAffect, int>;
    static const AffectTimeout s_affectTimeout;

    bool init();
    void issueLocalCharUpdate();

    QMutex networkLock{};
    std::unique_ptr<QThread> thread;
    std::unique_ptr<GroupAuthority> authority;
    std::unique_ptr<CGroupCommunicator> network;
    std::unique_ptr<CGroup> group;
};
