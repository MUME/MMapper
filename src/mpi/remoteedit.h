#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/macros.h"
#include "../proxy/GmcpMessage.h"
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

public:
    struct DraftInfo
    {
        QString fileName;
        QString title;
        RemoteSessionId sessionId;
        QDateTime lastModified;
    };

private:
    std::map<RemoteInternalId, std::shared_ptr<RemoteEditSession>> m_sessions;
    uint32_t m_greatestUsedId = 0;

public:
    explicit RemoteEdit(QObject *parent);
    ~RemoteEdit() final = default;

public:
    void onDisconnected();
    void recoverDrafts();
    void slot_parseGmcpInput(const GmcpMessage &msg);
    void raiseSession(size_t taskId);

    static QString getDraftDirectory();
    static QString provisionDraftFile(RemoteSessionId sessionId,
                                      const QString &title,
                                      const QString &content);
    static bool saveDraftAtomic(const QString &fileName, const QString &content);
    static void deleteDraft(const QString &fileName);
    static QList<DraftInfo> discoverDrafts();

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

    static QString encodeMetadata(RemoteSessionId sessionId, const QString &title);
    static bool decodeMetadata(const QString &fileName, RemoteSessionId &sessionId, QString &title);

signals:
    void sig_sendGmcp(const GmcpMessage &msg);

public:
    NODISCARD RemoteEditSession *getSessionByTaskId(size_t taskId) const;

public slots:
    void slot_remoteView(const QString &, const QString &);
    void slot_remoteEdit(const RemoteSessionId, const QString &, const QString &);
};
