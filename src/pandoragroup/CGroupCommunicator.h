#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <cstdint>
#include <QByteArray>
#include <QList>
#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QtCore>

#include "GroupSocket.h"
#include "groupaction.h"
#include "mmapper2group.h"

class CGroup;
class GroupAuthority;

class CGroupCommunicator : public QObject
{
    Q_OBJECT
public:
    explicit CGroupCommunicator(GroupManagerStateEnum mode, Mmapper2Group *parent);

    static constexpr const ProtocolVersion PROTOCOL_VERSION_103 = 103;
    static constexpr const ProtocolVersion PROTOCOL_VERSION_102 = 102;

    // TODO: password and encryption options
    enum class MessagesEnum {
        NONE, // Unused
        ACK,
        REQ_LOGIN,
        REQ_ACK,
        REQ_HANDSHAKE,
        REQ_INFO,
        PROT_VERSION, // Unused
        GTELL,
        STATE_LOGGED,
        STATE_KICKED,
        ADD_CHAR,
        REMOVE_CHAR,
        UPDATE_CHAR,
        RENAME_CHAR
    };

    GroupManagerStateEnum getMode() const { return mode; }
    void sendSelfRename(const QByteArray &, const QByteArray &);

    virtual void stop() = 0;
    virtual bool start() = 0;
    virtual void sendCharUpdate(const QVariantMap &map) = 0;
    virtual bool kickCharacter(const QByteArray &) = 0;

protected:
    void sendCharUpdate(GroupSocket *, const QVariantMap &);
    void sendMessage(GroupSocket *, MessagesEnum, const QByteArray & = "");
    void sendMessage(GroupSocket *, MessagesEnum, const QVariantMap &);

    virtual void sendGroupTellMessage(const QVariantMap &map) = 0;
    virtual void sendCharRename(const QVariantMap &map) = 0;

    QByteArray formMessageBlock(MessagesEnum message, const QVariantMap &data);
    CGroup *getGroup();
    GroupAuthority *getAuthority();

public slots:
    void incomingData(GroupSocket *, const QByteArray &);
    void sendGroupTell(const QByteArray &);
    void relayLog(const QString &);
    virtual void retrieveData(GroupSocket *, MessagesEnum, const QVariantMap &) = 0;
    virtual void connectionClosed(GroupSocket *) = 0;

signals:
    void messageBox(QString message);
    void scheduleAction(GroupAction *action);
    void gTellArrived(QVariantMap node);
    void sendLog(const QString &);

private:
    GroupManagerStateEnum mode = GroupManagerStateEnum::Off;
};
