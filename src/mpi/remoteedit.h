#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/macros.h"
#include "remoteeditsession.h"

#include <climits>
#include <map>
#include <memory>

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QtCore>
#include <QtGlobal>

class RemoteEditSession;

class RemoteEdit final : public QObject
{
    Q_OBJECT
    friend class RemoteEditSession;

public:
    explicit RemoteEdit(QObject *parent);
    ~RemoteEdit() final = default;

public slots:
    void slot_remoteView(const QString &, const QString &);
    void slot_remoteEdit(const RemoteSession &, const QString &, const QString &);
    void slot_onDisconnected();

signals:
    void sig_sendToSocket(const QByteArray &);

protected:
    void cancel(const RemoteEditSession *);
    void save(const RemoteEditSession *);

private:
    NODISCARD uint getInternalIdCount()
    {
        return greatestUsedId == UINT_MAX ? 0 : greatestUsedId + 1;
    }
    void addSession(const RemoteSession &, const QString &, const QString &);
    void removeSession(const RemoteEditSession *session);

    uint greatestUsedId = 0;
    std::map<uint, std::unique_ptr<RemoteEditSession>> m_sessions;
};
