#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/macros.h"
#include "../proxy/TaggedBytes.h"
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

class NODISCARD_QOBJECT RemoteEdit final : public QObject
{
    Q_OBJECT

private:
    friend class RemoteEditSession;

private:
    std::map<RemoteInternalId, std::unique_ptr<RemoteEditSession>> m_sessions;
    uint32_t m_greatestUsedId = 0;

public:
    explicit RemoteEdit(QObject *parent);
    ~RemoteEdit() final = default;

public:
    void onDisconnected();

protected:
    void cancel(const RemoteEditSession *);
    void save(const RemoteEditSession *);

private:
    NODISCARD uint32_t getInternalIdCount() const
    {
        return m_greatestUsedId == UINT_MAX ? 0 : m_greatestUsedId + 1;
    }
    void addSession(const RemoteSessionId, const QString &, const QString &);
    void removeSession(const RemoteEditSession &session);

private:
    void trySave(const RemoteEditSession &session);
    void sendToMume(const RemoteEditSession &session);
    void trySaveLocally(const RemoteEditSession &session);

signals:
    void sig_remoteEditCancel(const RemoteSessionId sessionId);
    void sig_remoteEditSave(const RemoteSessionId sessionId, const Latin1Bytes &content);

public slots:
    void slot_remoteView(const QString &, const QString &);
    void slot_remoteEdit(const RemoteSessionId, const QString &, const QString &);
};
