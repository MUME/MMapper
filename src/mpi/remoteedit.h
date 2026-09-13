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
class AnsiOstream;

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
    std::map<RemoteInternalId, std::unique_ptr<RemoteEditSession>> m_sessions;
    uint32_t m_greatestUsedId = 0;

public:
    explicit RemoteEdit(QObject *parent);
    ~RemoteEdit() final = default;

public:
    void onDisconnected();
    /// Called once at startup: auto-opens every persisted draft not already
    /// backed by an active session as a live recovery window.
    void recoverDrafts();
    void slot_parseGmcpInput(const GmcpMessage &msg);

    static QString getDraftDirectory();
    static QString provisionDraftFile(RemoteSessionId sessionId,
                                      const QString &title,
                                      const QString &content);
    static bool saveDraftAtomic(const QString &fileName, const QString &content);
    static void deleteDraft(const QString &fileName);
    static QList<DraftInfo> discoverDrafts();

public:
    /// Abort a session: sends a GMCP cancel if it's a live connected edit,
    /// then removes the in-memory session. Deletes the draft file for a
    /// live edit being cancelled (the user is explicitly abandoning it);
    /// a draft-recovery window's draft is left on disk so it can be
    /// recovered again later.
    void cancelEdit(RemoteEditSession *session);
    /// Explicit, user-requested, irreversible deletion of a draft's file,
    /// plus removal of the in-memory session if one exists.
    void discardDraft(const RemoteEditSession *session);
    /// Same, for a pending draft with no in-memory session backing it.
    void discardDraft(const DraftInfo &draft);
    /// Reopens a persisted draft (read from disk) as a live
    /// RemoteEditInternalSession window, regardless of the
    /// internalRemoteEditor setting.
    void recoverDraft(const DraftInfo &draft);
    /// Called once from MainWindow::closeEvent(): synchronously tears down
    /// every session (this is what actually stops any live external editor
    /// process and closes any open widget) without sending any GMCP message
    /// or deleting any draft file, so every open edit survives as a
    /// recoverable draft across a restart.
    void shutdown();

    /// Sessions/drafts not backed by an active session, for `_edits`/the
    /// "Remote Edits" menu.
    NODISCARD QList<DraftInfo> pendingDrafts() const;
    NODISCARD const std::map<RemoteInternalId, std::unique_ptr<RemoteEditSession>> &getSessions()
        const
    {
        return m_sessions;
    }

    void reportStatus(AnsiOstream &aos) const;
    NODISCARD bool reportStatus(AnsiOstream &aos, RemoteInternalId id) const;

protected:
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
    /// Emitted whenever a session is added or removed, so UI (the "Remote
    /// Edits" menu) can stay current without polling.
    void sig_sessionsChanged();

public slots:
    void slot_remoteView(const QString &, const QString &);
    void slot_remoteEdit(const RemoteSessionId, const QString &, const QString &);
};

/// Non-owning access to the single MainWindow-owned RemoteEdit instance, for
/// callers (like the `_edits` in-game command in AbstractParser) that have
/// no direct reference to it. Mirrors the async_tasks:: free-function
/// registry pattern (see AsyncTasks.h) used for the same reason.
namespace remote_edit {
void setInstance(RemoteEdit *instance);
extern void report_status(AnsiOstream &aos);
NODISCARD extern bool report_status(AnsiOstream &aos, uint32_t id);
NODISCARD extern bool cancel(uint32_t id);
NODISCARD extern bool discard(uint32_t id);
} // namespace remote_edit
