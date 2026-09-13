// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "remoteedit.h"

#include "../configuration/configuration.h"
#include "../global/AnsiOstream.h"
#include "../global/AnsiTextUtils.h"
#include "../global/SendToUser.h"
#include "../global/window_utils.h"
#include "remoteeditsession.h"

#include <cassert>
#include <memory>
#include <utility>

#include <QDateTime>
#include <QGuiApplication>
#include <QMessageBox>
#include <QMessageLogContext>
#include <QPushButton>
#include <QString>

namespace { // anonymous

const volatile bool g_prefixMessagesToUser = true;
constexpr const auto whiteOnCyan = getRawAnsi(AnsiColor16Enum::white, AnsiColor16Enum::cyan);
void notifyUserOfNewSession(const std::string_view article,
                            const std::string_view what,
                            const QString &title)
{
    global::sendToUser([&article, &what, &title](AnsiOstream &aos) {
        const auto color = whiteOnCyan;
        if (g_prefixMessagesToUser) {
            aos.writeWithColor(color.withBold(), "Info");
            aos.writeWithColor(color, ": ");
        }
        aos.writeWithColor(color, "MMapper is opening ");
        aos.writeWithColor(color, article);
        aos.writeWithColor(color, " ");
        aos.writeWithColor(color.withBold(), what);
        aos.writeWithColor(color, " window with title \"");
        aos.writeWithColor(color.withBold(), mmqt::toStdStringUtf8(title));
        aos.writeWithColor(color, "\"");
        aos.write("\n");
    });
}

void notifyUserOfSubmissionFailure(const QString &title, const QString &errorMsg)
{
    global::sendToUser([&title, &errorMsg](AnsiOstream &aos) {
        const auto color = whiteOnCyan;
        if (g_prefixMessagesToUser) {
            aos.writeWithColor(color.withBold(), "Info");
            aos.writeWithColor(color, ": ");
        }
        aos.writeWithColor(color, "Submission of \"");
        aos.writeWithColor(color.withBold(), mmqt::toStdStringUtf8(title));
        aos.writeWithColor(color, "\" to MUME failed: ");
        aos.writeWithColor(color.withBold(), mmqt::toStdStringUtf8(errorMsg));
        aos.writeWithColor(color,
                           ". It was kept as an unsent draft (Sidepanels > Remote Edits Panel).");
        aos.write("\n");
    });
}

} // namespace

RemoteEdit::RemoteEdit(QObject *const parent)
    : QObject(parent)
    , m_store(RemoteEditDraftStore::makeDefault())
{}

RemoteEdit::~RemoteEdit() = default;

void RemoteEdit::slot_remoteView(const QString &title, const QString &body)
{
    addSession(REMOTE_VIEW_SESSION_ID, title, body);
}

void RemoteEdit::slot_remoteEdit(const RemoteSessionId sessionId,
                                 const QString &title,
                                 const QString &body)
{
    addSession(sessionId, title, body);
}

void RemoteEdit::addSession(const RemoteSessionId sessionId,
                            const QString &title,
                            const QString &body)
{
    const bool isEdit = (sessionId != REMOTE_VIEW_SESSION_ID);
    if (isEdit) {
        notifyUserOfNewSession("an", "Editor", title);
    } else {
        notifyUserOfNewSession("a", "Viewer", title);
    }

    const auto offeredDraft = isEdit ? findPendingDraft(title) : std::nullopt;

#ifndef Q_OS_WASM
    if (offeredDraft && !getConfig().mumeClientProtocol.internalRemoteEditor) {
        // An external editor can't show a restore banner, so ask before
        // launching it, and seed its file with whichever text was chosen.
        auto *const dlg = new QMessageBox(
            QMessageBox::Question,
            tr("Recovered draft"),
            tr("An unsent draft of \"%1\" from %2 was recovered.\n\n"
               "Start the editor from the recovered draft, or from the text MUME just sent?")
                .arg(title, offeredDraft->lastModified.toString()),
            QMessageBox::NoButton,
            nullptr);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        QPushButton *const useDraft = dlg->addButton(tr("Recovered draft"), QMessageBox::AcceptRole);
        dlg->addButton(tr("Text from MUME"), QMessageBox::RejectRole);
        dlg->setDefaultButton(useDraft);
        const DraftInfo draft = *offeredDraft;
        connect(dlg,
                &QMessageBox::finished,
                this,
                [this, dlg, useDraft, sessionId, title, body, draft]() {
                    if (dlg->clickedButton() == static_cast<QAbstractButton *>(useDraft)) {
                        createSession(sessionId, title, readDraft(draft.key), std::nullopt);
                        deleteDraft(draft.key);
                    } else {
                        createSession(sessionId, title, body, std::nullopt);
                    }
                });
        dlg->open();
        return;
    }
#endif

    createSession(sessionId, title, body, offeredDraft);
}

void RemoteEdit::createSession(const RemoteSessionId sessionId,
                               const QString &title,
                               const QString &body,
                               const std::optional<DraftInfo> &offeredDraft)
{
    const auto internalId = RemoteInternalId{getInternalIdCount()};
    const bool isEdit = (sessionId != REMOTE_VIEW_SESSION_ID);
    std::unique_ptr<RemoteEditSession> session;

    const QString draftKey = isEdit ? deref(m_store).create(sessionId, title, body) : QString();

    if (getConfig().mumeClientProtocol.internalRemoteEditor) {
        session = std::make_unique<RemoteEditInternalSession>(internalId,
                                                              sessionId,
                                                              title,
                                                              body,
                                                              draftKey,
                                                              /*draftRecovery=*/false,
                                                              this);
    } else {
#ifndef Q_OS_WASM
        session = std::make_unique<RemoteEditExternalSession>(internalId,
                                                              sessionId,
                                                              title,
                                                              body,
                                                              draftKey,
                                                              this);
#else
        mmqt::showInformation(nullptr,
                              "External Editor Not Supported",
                              "Editing in an external editor is not supported on this platform.");
        return;
#endif
    }

    if (offeredDraft) {
        session->offerDraft(*offeredDraft);
    }

    m_sessions.insert(std::make_pair(internalId, std::move(session)));

    m_greatestUsedId = internalId.asUint32(); // Increment internalId counter
    emit sig_sessionsChanged();
    if (isEdit) {
        emit sig_draftsChanged();
    }
}

void RemoteEdit::removeSession(const RemoteEditSession &session)
{
    const auto internalId = session.getInternalId();
    const auto search = m_sessions.find(internalId);
    if (search != m_sessions.end()) {
        qDebug() << "Destroying RemoteEditSession" << internalId.asUint32();
        m_sessions.erase(search);
        emit sig_sessionsChanged();
    } else {
        qWarning() << "Unable to find" << internalId.asUint32() << "session to erase";
    }
}

void RemoteEdit::cancelEdit(RemoteEditSession *const pSession)
{
    auto &session = deref(pSession);

    // Only a connected live edit is truly abandoned here (MUME is told to
    // cancel, so the draft is deleted). A disconnected edit or a recovery
    // window closed the same way keeps its draft on disk for later recovery.
    if (session.isEditSession() && session.isConnected() && !session.isDraftRecovery()) {
        qDebug() << "Cancelling session" << session.getSessionId().asInt32();

        QJsonObject obj;
        obj["id"] = session.getSessionId().asInt32();
        QJsonDocument doc;
        doc.setObject(obj);
        GmcpJson json{QString::fromUtf8(doc.toJson())};
        GmcpMessage msg{GmcpMessageTypeEnum::MUME_CLIENT_CANCEL_EDIT, json};
        emit sig_sendGmcp(msg);
        deleteDraft(session.getDraftKey());
    } else {
        session.flushDraft();
    }
    removeSession(session);
}

void RemoteEdit::discardDraft(const RemoteEditSession *const pSession)
{
    auto &session = deref(pSession);
    deleteDraft(session.getDraftKey());
    removeSession(session);
}

void RemoteEdit::discardDraft(const DraftInfo &draft)
{
    deleteDraft(draft.key);
}

void RemoteEdit::save(const RemoteEditSession *const pSession)
{
    auto &session = deref(pSession);
    trySave(session);
    // If connected, the session lives on (with no window backing it) until
    // the MUME.Client.Write ack arrives; see slot_parseGmcpInput().
    if (!session.isConnected()) {
        removeSession(session);
    }
}

void RemoteEdit::trySave(const RemoteEditSession &session)
{
    if (!session.isEditSession()) {
        qWarning() << "Session" << session.getInternalId().asUint32()
                   << "was not an edit session and could not be saved";
        assert(false);
        return;
    }

    // Submit the edit session if we are still connected
    if (!session.isConnected()) {
        trySaveLocally(session);
    } else {
        sendToMume(session);
    }
}

void RemoteEdit::sendToMume(const RemoteEditSession &session)
{
    if (!session.isEditSession()) {
        std::abort();
    }

    qDebug() << "Saving session" << session.getSessionId().asInt32();
    // REVISIT: should we warn if this transformation modifies the content
    // (e.g. unicode transliteration, etc).
    auto latin1 = Latin1Bytes{
        mmqt::toQByteArrayLatin1(session.getContent())}; // MPI is always Latin1

    QJsonObject obj;
    obj["text"] = QString::fromLatin1(latin1.getQByteArray());
    obj["id"] = session.getSessionId().asInt32();
    QJsonDocument doc;
    doc.setObject(obj);
    GmcpJson json{QString::fromUtf8(doc.toJson())};
    GmcpMessage msg{GmcpMessageTypeEnum::MUME_CLIENT_WRITE, json};

    emit sig_sendGmcp(msg);

    // FR-4.4: Upon confirmed delivery success, delete local temporary file and unregister task.
    // Deletion is now handled in slot_parseGmcpInput for MUME_CLIENT_WRITE.
}

void RemoteEdit::trySaveLocally(const RemoteEditSession &session)
{
    // The draft on disk already holds the latest content (auto-save for the
    // internal editor; the editor's own save for an external one).
    qWarning() << "Session" << session.getInternalId().asUint32()
               << "submitted while disconnected - draft preserved";
    global::sendToUser([&session](AnsiOstream &aos) {
        const auto color = whiteOnCyan;
        if (g_prefixMessagesToUser) {
            aos.writeWithColor(color.withBold(), "Info");
            aos.writeWithColor(color, ": ");
        }
        aos.writeWithColor(color, "Not connected to MUME; \"");
        aos.writeWithColor(color.withBold(), mmqt::toStdStringUtf8(session.getTitle()));
        aos.writeWithColor(color,
                           "\" was kept as an unsent draft. Re-run the edit command once "
                           "reconnected to restore it.");
        aos.write("\n");
    });
}

void RemoteEdit::onDisconnected()
{
    for (const auto &pair : m_sessions) {
        const auto &id = pair.first;
        const auto &session = pair.second;
        if (session->isEditSession() && session->isConnected()) {
            qWarning() << "Session" << id.asUint32() << "marked as disconnected";
            session->setDisconnected();
        }
    }
}

void RemoteEdit::slot_parseGmcpInput(const GmcpMessage &msg)
{
    if (msg.isMumeClientEdit()) {
        auto doc = msg.getJsonDocument();
        if (!doc)
            return;
        auto optObj = doc->getObject();
        if (!optObj)
            return;
        auto &obj = *optObj;
        auto optId = obj.getInt("id");
        auto optTitle = obj.getString("title");
        auto optBody = obj.getString("text");
        if (optId && optTitle && optBody) {
            slot_remoteEdit(RemoteSessionId(*optId), *optTitle, *optBody);
        }
    } else if (msg.isMumeClientView()) {
        auto doc = msg.getJsonDocument();
        if (!doc)
            return;
        auto optObj = doc->getObject();
        if (!optObj)
            return;
        auto &obj = *optObj;
        auto optTitle = obj.getString("title");
        auto optBody = obj.getString("text");
        if (optTitle && optBody) {
            slot_remoteView(*optTitle, *optBody);
        }
    } else if (msg.isMumeClientWrite()) {
        auto doc = msg.getJsonDocument();
        if (!doc)
            return;
        auto optObj = doc->getObject();
        if (!optObj)
            return;
        auto &obj = *optObj;
        auto optId = obj.getInt("id");
        auto optResult = obj.getBool("result");
        auto optResultMsg = obj.getString("result");
        if (optId) {
            const auto sessionId = RemoteSessionId(*optId);
            for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
                // A recovered draft's session id belongs to a dead MUME session and
                // can never legitimately receive this ack.
                if (it->second->isDraftRecovery() || it->second->getSessionId() != sessionId) {
                    continue;
                }
                if (optResult && *optResult) {
                    qDebug() << "MUME.Client.Write success for session" << optId.value();
                    deleteDraft(it->second->getDraftKey());
                } else {
                    const QString errorMsg = optResultMsg.value_or("unknown error");
                    qWarning() << "MUME.Client.Write failed for session" << optId.value() << ":"
                               << errorMsg;
                    notifyUserOfSubmissionFailure(it->second->getTitle(), errorMsg);
                    // Draft is kept so it can be restored by re-running the edit command.
                }
                removeSession(*(it->second));
                break;
            }
        }
    } else if (msg.isMumeClientCancelEdit()) {
        auto doc = msg.getJsonDocument();
        if (!doc)
            return;
        auto optObj = doc->getObject();
        if (!optObj)
            return;
        auto &obj = *optObj;
        auto optId = obj.getInt("id");
        auto optResult = obj.getBool("result");
        if (optId) {
            // The session is already gone by the time this ack arrives -- cancelEdit()
            // removes it (and deletes its draft) synchronously when the user cancels.
            // This is purely informational logging.
            if (optResult && *optResult) {
                qDebug() << "MUME.Client.CancelEdit success for session" << optId.value();
            } else {
                qWarning() << "MUME.Client.CancelEdit failed for session" << optId.value();
            }
        }
    } else if (msg.isCoreGoodbye()) {
        onDisconnected();
    }
}

QList<RemoteEdit::DraftInfo> RemoteEdit::pendingDrafts() const
{
    QList<DraftInfo> pending;
    for (const auto &draft : deref(m_store).list()) {
        bool active = false;
        for (const auto &pair : m_sessions) {
            if (pair.second->getDraftKey() == draft.key) {
                active = true;
                break;
            }
        }
        if (!active) {
            pending.append(draft);
        }
    }
    return pending;
}

std::optional<RemoteEdit::DraftInfo> RemoteEdit::findPendingDraft(const QString &title) const
{
    std::optional<DraftInfo> best;
    for (const auto &draft : pendingDrafts()) {
        if (draft.title == title && (!best || draft.lastModified > best->lastModified)) {
            best = draft;
        }
    }
    return best;
}

void RemoteEdit::announcePendingDrafts() const
{
    const auto drafts = pendingDrafts();
    if (drafts.isEmpty()) {
        return;
    }
    global::sendToUser([&drafts](AnsiOstream &aos) {
        const auto color = whiteOnCyan;
        if (g_prefixMessagesToUser) {
            aos.writeWithColor(color.withBold(), "Info");
            aos.writeWithColor(color, ": ");
        }
        aos.writeWithColor(color, "MMapper has ");
        aos.writeWithColor(color.withBold(), std::to_string(drafts.size()));
        aos.writeWithColor(color, drafts.size() == 1 ? " unsent draft" : " unsent drafts");
        aos.writeWithColor(color,
                           " from a previous session (Sidepanels > Remote Edits Panel, or _edits).");
        aos.write("\n");
    });
}

void RemoteEdit::viewDraft(const DraftInfo &draft)
{
    for (const auto &pair : m_sessions) {
        if (pair.second->getDraftKey() == draft.key) {
            pair.second->focus();
            return;
        }
    }

    const auto internalId = RemoteInternalId{getInternalIdCount()};
    auto session = std::make_unique<RemoteEditInternalSession>(internalId,
                                                               REMOTE_VIEW_SESSION_ID,
                                                               draft.title,
                                                               readDraft(draft.key),
                                                               draft.key,
                                                               /*draftRecovery=*/true,
                                                               this);
    session->setDisconnected();

    m_sessions.insert(std::make_pair(internalId, std::move(session)));
    m_greatestUsedId = internalId.asUint32();
    emit sig_sessionsChanged();
}

void RemoteEdit::shutdown()
{
    // Destroying each session synchronously closes its widget and (for a
    // live external session) terminates the child process -- see
    // ~RemoteEditExternalSession. No GMCP message is sent and no draft is
    // deleted, so every open edit remains recoverable on next launch.
    m_sessions.clear();
}

void RemoteEdit::reportStatus(AnsiOstream &aos) const
{
    aos.write("Remote edits:\n");
    for (const auto &pair : m_sessions) {
        std::ignore = reportStatus(aos, pair.first);
    }
    const auto pending = pendingDrafts();
    for (const auto &draft : pending) {
        aos.write("  (pending) ");
        aos.write(mmqt::toStdStringUtf8(draft.title));
        aos.write(" -- unsent draft, last modified ");
        aos.write(mmqt::toStdStringUtf8(draft.lastModified.toString()));
        aos.write("\n");
    }
    aos.write("Total: ");
    aos.write(m_sessions.size());
    aos.write(" open, ");
    aos.write(static_cast<size_t>(pending.size()));
    aos.write(" unsent draft(s).\n");
}

bool RemoteEdit::reportStatus(AnsiOstream &aos, const RemoteInternalId id) const
{
    const auto it = m_sessions.find(id);
    if (it == m_sessions.end()) {
        return false;
    }
    const auto &session = *it->second;

    aos.write("  #");
    aos.write(id.asUint32());
    aos.write(" \"");
    aos.write(mmqt::toStdStringUtf8(session.getTitle()));
    aos.write("\" -- ");
    if (session.isDraftRecovery()) {
        aos.write("viewing unsent draft (read-only)");
    } else if (!session.isEditSession()) {
        aos.write("viewing");
    } else if (session.isConnected()) {
        aos.write("editing (connected)");
    } else {
        aos.write("editing (disconnected -- draft preserved)");
    }
    aos.write("\n");
    return true;
}

void RemoteEdit::deleteDraft(const QString &key)
{
    if (key.isEmpty()) {
        return;
    }
    deref(m_store).remove(key);
    emit sig_draftsChanged();
}

namespace remote_edit {

namespace {
RemoteEdit *g_instance = nullptr;
} // namespace

void setInstance(RemoteEdit *const instance)
{
    g_instance = instance;
}

void report_status(AnsiOstream &aos)
{
    if (g_instance == nullptr) {
        aos.write("Error: RemoteEdit is not available.\n");
        return;
    }
    g_instance->reportStatus(aos);
}

bool report_status(AnsiOstream &aos, const uint32_t id)
{
    if (g_instance == nullptr) {
        aos.write("Error: RemoteEdit is not available.\n");
        return false;
    }
    if (!g_instance->reportStatus(aos, RemoteInternalId{id})) {
        aos.write("Error: Invalid remote edit id.\n");
        return false;
    }
    return true;
}

bool cancel(const uint32_t id)
{
    if (g_instance == nullptr) {
        return false;
    }
    const auto &sessions = g_instance->getSessions();
    const auto it = sessions.find(RemoteInternalId{id});
    if (it == sessions.end()) {
        return false;
    }
    g_instance->cancelEdit(it->second.get());
    return true;
}

bool discard(const uint32_t id)
{
    if (g_instance == nullptr) {
        return false;
    }
    const auto &sessions = g_instance->getSessions();
    const auto it = sessions.find(RemoteInternalId{id});
    if (it == sessions.end()) {
        return false;
    }
    // A live edit has no draft-only state to discard; route it through cancelEdit()
    // (which also sends the GMCP cancel and deletes the draft) instead.
    if (it->second->isDraftRecovery()) {
        g_instance->discardDraft(it->second.get());
    } else {
        g_instance->cancelEdit(it->second.get());
    }
    return true;
}

} // namespace remote_edit
