#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/macros.h"
#include "../global/utils.h"
#include "../proxy/GmcpMessage.h"
#include "../proxy/TaggedBytes.h"
#include "RemoteEditDraftStore.h"
#include "remoteeditsession.h"

#include <climits>
#include <map>
#include <optional>
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
    using DraftInfo = RemoteEditDraftInfo;

private:
    std::unique_ptr<RemoteEditDraftStore> m_store;
    std::map<RemoteInternalId, std::unique_ptr<RemoteEditSession>> m_sessions;
    uint32_t m_greatestUsedId = 0;

public:
    explicit RemoteEdit(QObject *parent);
    ~RemoteEdit() final;

public:
    void onDisconnected();
    /// One line in the game output if unsent drafts exist; called when a
    /// connection comes up, since nothing can hear it before then.
    void announcePendingDrafts() const;
    void slot_parseGmcpInput(const GmcpMessage &msg);

    NODISCARD RemoteEditDraftStore &getDraftStore() { return deref(m_store); }
    NODISCARD QString readDraft(const QString &key) const { return deref(m_store).read(key); }
    /// Removes the draft and tells listeners.
    void deleteDraft(const QString &key);

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
    /// Opens a pending draft as a read-only page (always internal, whatever
    /// the internalRemoteEditor setting). MUME no longer has the edit open,
    /// so the only way to send it is to re-run the edit command and accept
    /// the restore offer that a matching title then triggers.
    void viewDraft(const DraftInfo &draft);
    /// Called once from MainWindow::closeEvent(): synchronously tears down
    /// every session (this is what actually stops any live external editor
    /// process and closes any open widget) without sending any GMCP message
    /// or deleting any draft file, so every open edit survives as a
    /// recoverable draft across a restart.
    void shutdown();

    /// Drafts on disk not backed by an active session.
    NODISCARD QList<DraftInfo> pendingDrafts() const;
    /// Most recently modified pending draft with this title, if any.
    NODISCARD std::optional<DraftInfo> findPendingDraft(const QString &title) const;
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
    void createSession(RemoteSessionId sessionId,
                       const QString &title,
                       const QString &body,
                       const std::optional<DraftInfo> &offeredDraft);
    void removeSession(const RemoteEditSession &session);

private:
    void trySave(const RemoteEditSession &session);
    void sendToMume(const RemoteEditSession &session);
    void trySaveLocally(const RemoteEditSession &session);

signals:
    void sig_sendGmcp(const GmcpMessage &msg);
    /// Emitted whenever a session is added or removed.
    void sig_sessionsChanged();
    /// Emitted whenever a draft file is created or deleted.
    void sig_draftsChanged();

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
