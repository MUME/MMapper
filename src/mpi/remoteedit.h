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

private:
    friend class RemoteEditSession;

private:
    std::map<uint32_t, std::unique_ptr<RemoteEditSession>> m_sessions;
    uint32_t m_greatestUsedId = 0;

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
    NODISCARD uint32_t getInternalIdCount() const
    {
        return m_greatestUsedId == UINT_MAX ? 0 : m_greatestUsedId + 1;
    }
    void addSession(const RemoteSession &, const QString &, const QString &);
    void removeSession(const RemoteEditSession *session);
};
