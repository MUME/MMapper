// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "remoteedit.h"

#include "../configuration/configuration.h"
#include "../global/AnsiOstream.h"
#include "../global/AnsiTextUtils.h"
#include "../global/SendToUser.h"
#include "../global/io.h"
#include "../global/random.h"
#include "../global/window_utils.h"
#include "remoteeditsession.h"

#include <cassert>
#include <memory>
#include <sstream>
#include <utility>

#include <QClipboard>
#include <QDateTime>
#include <QFileDialog>
#include <QGuiApplication>
#include <QMessageBox>
#include <QMessageLogContext>
#include <QRegularExpression>
#include <QSaveFile>
#include <QString>
#include <QUrl>

namespace { // anonymous

const volatile bool g_prefixMessagesToUser = true;
constexpr const auto whiteOnCyan = getRawAnsi(AnsiColor16Enum::white, AnsiColor16Enum::cyan);
constexpr const std::string_view VALID_RANDOM_CHARS
    = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

NODISCARD std::string randomString(const int length)
{
    std::ostringstream os;
    for (int i = 0; i < length; ++i) {
        os << VALID_RANDOM_CHARS[getRandom(VALID_RANDOM_CHARS.length())];
    }
    return os.str();
}

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
        aos.writeWithColor(color, ". The draft is preserved under Tools > Remote Edits.");
        aos.write("\n");
    });
}

} // namespace

RemoteEdit::RemoteEdit(QObject *const parent)
    : QObject(parent)
{}

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
    const auto internalId = RemoteInternalId{getInternalIdCount()};
    const bool isEdit = (sessionId != REMOTE_VIEW_SESSION_ID);
    std::unique_ptr<RemoteEditSession> session;

    if (isEdit) {
        notifyUserOfNewSession("an", "Editor", title);
    } else {
        notifyUserOfNewSession("a", "Viewer", title);
    }

    const QString draftFileName = isEdit ? provisionDraftFile(sessionId, title, body) : QString();

    if (getConfig().mumeClientProtocol.internalRemoteEditor) {
        session = std::make_unique<RemoteEditInternalSession>(internalId,
                                                              sessionId,
                                                              title,
                                                              body,
                                                              draftFileName,
                                                              /*draftRecovery=*/false,
                                                              this);
    } else {
#ifndef Q_OS_WASM
        session = std::make_unique<RemoteEditExternalSession>(internalId,
                                                              sessionId,
                                                              title,
                                                              body,
                                                              draftFileName,
                                                              this);
#else
        mmqt::showInformation(nullptr,
                              "External Editor Not Supported",
                              "Editing in an external editor is not supported on this platform.");
        return;
#endif
    }

    m_sessions.insert(std::make_pair(internalId, std::move(session)));

    m_greatestUsedId = internalId.asUint32(); // Increment internalId counter
    emit sig_sessionsChanged();
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
        deleteDraft(session.getDraftFileName());
    } else {
        session.flushDraft();
    }
    removeSession(session);
}

void RemoteEdit::discardDraft(const RemoteEditSession *const pSession)
{
    auto &session = deref(pSession);
    deleteDraft(session.getDraftFileName());
    removeSession(session);
}

void RemoteEdit::discardDraft(const DraftInfo &draft)
{
    deleteDraft(draft.fileName);
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
    if (!session.isEditSession()) {
        assert(false);
    }

    auto *dlg = new QMessageBox(
        QMessageBox::Information,
        "MUME Disconnected",
        "The connection to MUME was lost. Your changes have been preserved as a draft in the "
        "MMapper/Editor directory and are available under Tools > Remote Edits for recovery.",
        QMessageBox::StandardButtons{QMessageBox::Ok},
        nullptr);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    const auto id = session.getInternalId().asUint32();
    dlg->open();
    qWarning() << "Session" << id << "marked as disconnected - draft preserved";
}

void RemoteEdit::onDisconnected()
{
    for (const auto &pair : m_sessions) {
        const auto &id = pair.first;
        const auto &session = pair.second;
        if (session->isEditSession()) {
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
                    deleteDraft(it->second->getDraftFileName());
                } else {
                    const QString errorMsg = optResultMsg.value_or("unknown error");
                    qWarning() << "MUME.Client.Write failed for session" << optId.value() << ":"
                               << errorMsg;
                    notifyUserOfSubmissionFailure(it->second->getTitle(), errorMsg);
                    // Draft is kept -- the edit remains recoverable from the Remote Edits menu.
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
    for (const auto &draft : discoverDrafts()) {
        bool active = false;
        for (const auto &pair : m_sessions) {
            if (pair.second->getDraftFileName() == draft.fileName) {
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

void RemoteEdit::recoverDrafts()
{
    const auto drafts = pendingDrafts();
    if (drafts.isEmpty()) {
        return;
    }
    qInfo() << "Scanning for recovered drafts in" << getDraftDirectory();
    for (const auto &draft : drafts) {
        recoverDraft(draft);
    }
}

void RemoteEdit::recoverDraft(const DraftInfo &draft)
{
    for (const auto &pair : m_sessions) {
        if (pair.second->getDraftFileName() == draft.fileName) {
            // Already open (e.g. re-triggered from the Remote Edits menu) --
            // just bring its window to the front instead of opening a
            // duplicate.
            pair.second->focus();
            return;
        }
    }

    qInfo() << "Recovering draft:" << draft.fileName << "title:" << draft.title;

    QString content;
    const QString fullPath = QDir(getDraftDirectory()).absoluteFilePath(draft.fileName);
    if (QFile file(fullPath); file.open(QFile::ReadOnly)) {
        content = QString::fromLatin1(file.readAll()); // MPI is always Latin1
    } else {
        qWarning() << "Unable to read draft" << fullPath;
    }

    const auto internalId = RemoteInternalId{getInternalIdCount()};
    auto session = std::make_unique<RemoteEditInternalSession>(internalId,
                                                               draft.sessionId,
                                                               draft.title,
                                                               content,
                                                               draft.fileName,
                                                               /*draftRecovery=*/true,
                                                               this);
    session->setDisconnected(); // Recovered drafts are naturally disconnected

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
        aos.write(" -- recovered draft, last modified ");
        aos.write(mmqt::toStdStringUtf8(draft.lastModified.toString()));
        aos.write("\n");
    }
    aos.write("Total: ");
    aos.write(m_sessions.size());
    aos.write(" open, ");
    aos.write(static_cast<size_t>(pending.size()));
    aos.write(" pending recovered draft(s).\n");
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
        aos.write("recovered draft");
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

QString RemoteEdit::getDraftDirectory()
{
    QString dir = getConfig().mumeClientProtocol.editorDirectory;
    QDir().mkpath(dir);
    return dir;
}

QString RemoteEdit::encodeMetadata(RemoteSessionId sessionId, const QString &title)
{
    // MUME reuses small session ids, so the timestamp -- not just the id --
    // is needed to keep concurrent/successive drafts from colliding.
    const QByteArray encoded = QUrl::toPercentEncoding(title);
    QByteArray safeTitle = encoded.left(50);
    if (safeTitle.size() < encoded.size()) {
        // Don't split a "%XX" escape at the truncation boundary.
        const qsizetype lastPercent = safeTitle.lastIndexOf('%');
        if (lastPercent >= 0 && safeTitle.size() - lastPercent < 3) {
            safeTitle.truncate(lastPercent);
        }
    }
    return QString("draft_%1_%2_%3.txt")
        .arg(sessionId.asInt32())
        .arg(QDateTime::currentMSecsSinceEpoch())
        .arg(QString::fromLatin1(safeTitle));
}

bool RemoteEdit::decodeMetadata(const QString &fileName, RemoteSessionId &sessionId, QString &title)
{
    static const QRegularExpression re("^draft_(-?\\d+)_(\\d+)_(.*)\\.txt$");
    QRegularExpressionMatch match = re.match(fileName);
    if (match.hasMatch()) {
        sessionId = RemoteSessionId(match.captured(1).toInt());
        title = QUrl::fromPercentEncoding(match.captured(3).toUtf8());
        return true;
    }
    return false;
}

QString RemoteEdit::provisionDraftFile(RemoteSessionId sessionId,
                                       const QString &title,
                                       const QString &content)
{
    QString dir = getDraftDirectory();
    QString fileName = encodeMetadata(sessionId, title);
    QString fullPath = QDir(dir).absoluteFilePath(fileName);

    if (QFile::exists(fullPath)) {
        // Extremely unlikely (would require two provisions in the same millisecond
        // for the same session id), but never truncate someone else's draft.
        fileName = fileName.chopped(4) + "_" + mmqt::toQStringUtf8(randomString(5)) + ".txt";
        fullPath = QDir(dir).absoluteFilePath(fileName);
    }

    QFile file(fullPath);
    if (file.open(QFile::WriteOnly | QFile::Text)) {
        file.write(mmqt::toQByteArrayLatin1(content));
        file.flush();
        std::ignore = io::fsyncNoexcept(file);
        file.close();
        return fileName;
    }
    return QString();
}

bool RemoteEdit::saveDraftAtomic(const QString &fileName, const QString &content)
{
    QString fullPath = QDir(getDraftDirectory()).absoluteFilePath(fileName);
    QSaveFile file(fullPath);
    if (file.open(QFile::WriteOnly | QFile::Text)) {
        file.write(mmqt::toQByteArrayLatin1(content));
        return file.commit();
    }
    return false;
}

void RemoteEdit::deleteDraft(const QString &fileName)
{
    if (fileName.isEmpty())
        return;
    QFile::remove(QDir(getDraftDirectory()).absoluteFilePath(fileName));
}

QList<RemoteEdit::DraftInfo> RemoteEdit::discoverDrafts()
{
    QList<DraftInfo> drafts;
    QDir dir(getDraftDirectory());
    QStringList files = dir.entryList({"draft_*.txt"}, QDir::Files);

    for (const QString &fileName : files) {
        RemoteSessionId sid;
        QString title;
        if (decodeMetadata(fileName, sid, title)) {
            QFileInfo info(dir.absoluteFilePath(fileName));
            drafts.append({fileName, title, sid, info.lastModified()});
        }
    }
    return drafts;
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
