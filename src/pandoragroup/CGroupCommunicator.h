#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Dmitrijs Barbarins <lachupe@gmail.com> (Azazello)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <cstdint>
#include <memory>
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
    enum class NODISCARD MessagesEnum {
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

    NODISCARD GroupManagerStateEnum getMode() const { return mode; }

private:
    virtual void virt_stop() = 0;
    NODISCARD virtual bool virt_start() = 0;

public:
    void stop() { virt_stop(); }
    NODISCARD bool start() { return virt_start(); }

protected:
    void sendCharUpdate(GroupSocket *, const QVariantMap &);
    void sendMessage(GroupSocket *, MessagesEnum, const QByteArray & = "");
    void sendMessage(GroupSocket *, MessagesEnum, const QVariantMap &);

    NODISCARD QByteArray formMessageBlock(MessagesEnum message, const QVariantMap &data);
    NODISCARD CGroup *getGroup();
    NODISCARD GroupAuthority *getAuthority();

private:
    virtual void virt_connectionClosed(GroupSocket *) = 0;
    virtual void virt_kickCharacter(const QByteArray &) = 0;
    virtual void virt_retrieveData(GroupSocket *, MessagesEnum, const QVariantMap &) = 0;
    virtual void virt_sendCharRename(const QVariantMap &map) = 0;
    virtual void virt_sendCharUpdate(const QVariantMap &map) = 0;
    virtual void virt_sendGroupTellMessage(const QVariantMap &map) = 0;

public slots:
    void slot_connectionClosed(GroupSocket *sock) { virt_connectionClosed(sock); }
    void slot_kickCharacter(const QByteArray &msg) { virt_kickCharacter(msg); }
    void slot_retrieveData(GroupSocket *sock, MessagesEnum msg, const QVariantMap &var)
    {
        virt_retrieveData(sock, msg, var);
    }
    void slot_sendCharRename(const QVariantMap &map) { virt_sendCharRename(map); }
    void slot_sendCharUpdate(const QVariantMap &map) { virt_sendCharUpdate(map); }
    void slot_sendGroupTellMessage(const QVariantMap &map) { virt_sendGroupTellMessage(map); }

protected:
    void messageBox(const QString &message) { emit sig_messageBox(message); }
    void scheduleAction(std::shared_ptr<GroupAction> action) { emit sig_scheduleAction(action); }
    void gTellArrived(QVariantMap node) { emit sig_gTellArrived(node); }
    void sendLog(const QString &msg) { emit sig_sendLog(msg); }

public slots:
    void slot_incomingData(GroupSocket *, const QByteArray &);
    void slot_sendGroupTell(const QByteArray &);
    void slot_relayLog(const QString &);
    void slot_sendSelfRename(const QByteArray &, const QByteArray &);

signals:
    void sig_messageBox(QString message);
    void sig_scheduleAction(std::shared_ptr<GroupAction> action);
    void sig_gTellArrived(QVariantMap node);
    void sig_sendLog(const QString &);

private:
    GroupManagerStateEnum mode = GroupManagerStateEnum::Off;
};
