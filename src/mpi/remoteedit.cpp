// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "remoteedit.h"

#include "../configuration/configuration.h"
#include "../global/AsyncTasks.h"
#include "../global/Consts.h"
#include "../global/io.h"
#include "../global/window_utils.h"
#include "remoteeditsession.h"

#include <cassert>
#include <memory>
#include <utility>

#include <QClipboard>
#include <QFileDialog>
#include <QGuiApplication>
#include <QMessageBox>
#include <QMessageLogContext>
#include <QRegularExpression>
#include <QSaveFile>
#include <QString>
#include <QUrl>

using char_consts::C_NEWLINE;

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
    std::shared_ptr<RemoteEditSession> session;

    if (getConfig().mumeClientProtocol.internalRemoteEditor) {
        session = std::make_shared<RemoteEditInternalSession>(internalId,
                                                              sessionId,
                                                              title,
                                                              body,
                                                              this);
    } else {
#ifndef Q_OS_WASM
        session = std::make_shared<RemoteEditExternalSession>(internalId,
                                                              sessionId,
                                                              title,
                                                              body,
                                                              this);
#else
        mmqt::showInformation(nullptr,
                              "External Editor Not Supported",
                              "Editing in an external editor is not supported on this platform.");
        return;
#endif
    }

    if (isEdit) {
        QString fileName = provisionDraftFile(sessionId, title, body);
        session->setDraftFileName(fileName);

        std::weak_ptr<RemoteEditSession> weakSession = session;
        auto handle = async_tasks::startAsyncTask(
            AsyncTaskTypeEnum::RemoteEdit,
            AllowCancelEnum::Allow,
            mmqt::toStdStringUtf8(QString("RemoteEdit: %1").arg(title)),
            [weakSession, title](ProgressCounter &pc) {
                pc.setNewTask(ProgressMsg{QString("Editing %1").arg(title)}, 100);
                while (true) {
                    auto pSession = weakSession.lock();
                    if (!pSession || pSession->shouldStopTask()) {
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    if (pc.hasRequestedCancel()) {
                        QMetaObject::invokeMethod(pSession.get(),
                                                  "slot_onCancel",
                                                  Qt::QueuedConnection);
                        break;
                    }
                }
            },
            []() {});
        session->setAsyncTask(handle);
    }

    m_sessions.insert(std::make_pair(internalId, std::move(session)));

    m_greatestUsedId = internalId.asUint32(); // Increment internalId counter
}

void RemoteEdit::removeSession(const RemoteEditSession &session)
{
    const_cast<RemoteEditSession &>(session).stopTask();
    const auto internalId = session.getInternalId();
    const auto search = m_sessions.find(internalId);
    if (search != m_sessions.end()) {
        qDebug() << "Destroying RemoteEditSession" << internalId.asUint32();
        m_sessions.erase(search);

        // Ensure drafts from closed disconnected sessions appear as recovered tasks immediately.
        QTimer::singleShot(0, this, [this]() { recoverDrafts(); });
    } else {
        qWarning() << "Unable to find" << internalId.asUint32() << "session to erase";
    }
}

void RemoteEdit::cancel(const RemoteEditSession *const pSession)
{
    auto &session = deref(pSession);

    bool explicitDiscard = false;
    if (auto handle = session.getAsyncTask()) {
        if (handle->getProgressCounter().hasRequestedCancel()) {
            explicitDiscard = true;
        }
    }

    if (session.isEditSession() && session.isConnected()) {
        qDebug() << "Cancelling session" << session.getSessionId().asInt32();

        QJsonObject obj;
        obj["id"] = session.getSessionId().asInt32();
        QJsonDocument doc;
        doc.setObject(obj);
        GmcpJson json{QString::fromUtf8(doc.toJson())};
        GmcpMessage msg{GmcpMessageTypeEnum::MUME_CLIENT_CANCEL_EDIT, json};

        if (auto handle = session.getAsyncTask()) {
            handle->getProgressCounter().setCurrentTask(ProgressMsg{"Canceling edit..."});
        }

        emit sig_sendGmcp(msg);

        if (explicitDiscard) {
            // Explicitly requested task deletion: be aggressive and remove now.
            deleteDraft(session.getDraftFileName());
            removeSession(session);
        }
    } else if (session.isEditSession()) {
        if (explicitDiscard) {
            // FR-6.4: Explicit deletion command for recovered/disconnected task
            deleteDraft(session.getDraftFileName());
        }
        // Transition to recovered state by removing active session; recoverDrafts() trigger will pick it up.
        removeSession(session);
    } else {
        // Not an edit session: just remove.
        removeSession(session);
    }
}

void RemoteEdit::save(const RemoteEditSession *const pSession)
{
    auto &session = deref(pSession);
    trySave(session);
    // We do not call removeSession here if connected; we wait for the server's confirmation.
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

    if (auto handle = session.getAsyncTask()) {
        handle->getProgressCounter().setCurrentTask(ProgressMsg{"Submitting changes..."});
    }

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
        "MMapper/Editor directory and are available in the Tasks panel for recovery. "
        "The content has also been copied to your clipboard.",
        QMessageBox::StandardButtons{QMessageBox::Ok},
        nullptr);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    const auto id = session.getInternalId().asUint32();
    const auto body = session.getContent();
    dlg->open();
    QGuiApplication::clipboard()->setText(body);
    qWarning() << "Session" << id << "content was copied to the clipboard";

    if (auto handle = session.getAsyncTask()) {
        handle->getProgressCounter().setCurrentTask(
            ProgressMsg{"Disconnected - Changes copied to clipboard"});
    }
}

void RemoteEdit::onDisconnected()
{
    for (const auto &pair : m_sessions) {
        const auto &id = pair.first;
        const auto &session = pair.second;
        if (session->isEditSession()) {
            qWarning() << "Session" << id.asUint32() << "marked as disconnected";
            session->setDisconnected();
            if (auto handle = session->getAsyncTask()) {
                handle->getProgressCounter().setCurrentTask(
                    ProgressMsg{"Disconnected - Draft preserved"});
            }
        }
    }
}

void RemoteEdit::raiseSession(size_t taskId)
{
    if (auto *session = getSessionByTaskId(taskId)) {
        // If it's an external session that is still running, inform the user.
        if (auto *ext = dynamic_cast<RemoteEditExternalSession *>(session)) {
            if (ext->isRunning()) {
                QMessageBox::information(nullptr,
                                         "External Editor Active",
                                         QString("An external editor is already open for \"%1\". "
                                                 "Please switch to it to continue editing.")
                                             .arg(ext->getTitle()));
                return;
            }
        }
        session->raise();
    }
}

RemoteEditSession *RemoteEdit::getSessionByTaskId(size_t taskId) const
{
    for (const auto &pair : m_sessions) {
        if (pair.second->isEditSession()) {
            if (auto handle = pair.second->getAsyncTask()) {
                if (handle->getId() == taskId) {
                    return pair.second.get();
                }
            }
        }
    }
    return nullptr;
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
                if (it->second->getSessionId() == sessionId) {
                    if (optResult && *optResult) {
                        qDebug() << "MUME.Client.Write success for session" << optId.value();
                        deleteDraft(it->second->getDraftFileName());
                        removeSession(*(it->second));
                    } else if (auto handle = it->second->getAsyncTask()) {
                        const QString msg = optResultMsg.value_or("unknown error");
                        handle->getProgressCounter().setCurrentTask(
                            ProgressMsg{QString("Submission failed: %1").arg(msg)});
                    }
                    break;
                }
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
            const auto sessionId = RemoteSessionId(*optId);
            for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
                if (it->second->getSessionId() == sessionId) {
                    if (optResult && *optResult) {
                        qDebug() << "MUME.Client.CancelEdit success for session" << optId.value();
                        deleteDraft(it->second->getDraftFileName());
                        removeSession(*(it->second));
                    } else if (auto handle = it->second->getAsyncTask()) {
                        handle->getProgressCounter().setCurrentTask(ProgressMsg{"Cancel failed"});
                    }
                    break;
                }
            }
        }
    } else if (msg.isCoreGoodbye()) {
        onDisconnected();
    }
}

void RemoteEdit::recoverDrafts()
{
    auto drafts = discoverDrafts();
    if (drafts.isEmpty()) {
        return;
    }
    qInfo() << "Scanning for recovered drafts in" << getDraftDirectory();
    for (const auto &draft : drafts) {
        // Check if this draft is already being managed by an active session
        bool active = false;
        for (const auto &pair : m_sessions) {
            if (pair.second->getDraftFileName() == draft.fileName) {
                active = true;
                break;
            }
        }

        if (!active) {
            qInfo() << "Recovering draft:" << draft.fileName << "title:" << draft.title;

            // FR-5.2: Discover files lacking active session must be registered as recovered tasks.
            // FR-5.3: Recovered tasks must be strictly flagged as non-sendable from raw state.
            // Register this as a "recovered" session in our local map.
            const auto internalId = RemoteInternalId{getInternalIdCount()};
            auto session = std::make_shared<RemoteEditSession>(internalId,
                                                               draft.sessionId,
                                                               draft.title,
                                                               this);
            std::weak_ptr<RemoteEditSession> weakSession = session;

            auto handle = async_tasks::startAsyncTask(
                AsyncTaskTypeEnum::RemoteEdit,
                AllowCancelEnum::Allow,
                mmqt::toStdStringUtf8(QString("Recovered: %1").arg(draft.title)),
                [weakSession, draft](ProgressCounter &pc) {
                    pc.setNewTask(ProgressMsg{QString("Recovered draft from %1")
                                                  .arg(draft.lastModified.toString())},
                                  100);
                    while (true) {
                        auto pSession = weakSession.lock();
                        if (!pSession || pSession->shouldStopTask()) {
                            break;
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        if (pc.hasRequestedCancel()) {
                            QMetaObject::invokeMethod(pSession.get(),
                                                      "slot_onCancel",
                                                      Qt::QueuedConnection);
                            break;
                        }
                    }
                },
                []() {});

            session->setDraftFileName(draft.fileName);
            session->setAsyncTask(handle);
            session->setDisconnected(); // Recovered drafts are naturally disconnected

            m_sessions.insert(std::make_pair(internalId, std::move(session)));
            m_greatestUsedId = internalId.asUint32();
        }
    }
}

QString RemoteEdit::getDraftDirectory()
{
    QString dir = getConfig().mumeClientProtocol.editorDirectory;
    QDir().mkpath(dir);
    return dir;
}

QString RemoteEdit::encodeMetadata(RemoteSessionId sessionId, const QString &title)
{
    QString safeTitle = QUrl::toPercentEncoding(title).mid(0, 50);
    return QString("draft_%1_%2.txt").arg(sessionId.asInt32()).arg(safeTitle);
}

bool RemoteEdit::decodeMetadata(const QString &fileName, RemoteSessionId &sessionId, QString &title)
{
    static const QRegularExpression re("^draft_(-?\\d+)_(.*)\\.txt$");
    QRegularExpressionMatch match = re.match(fileName);
    if (match.hasMatch()) {
        sessionId = RemoteSessionId(match.captured(1).toInt());
        title = QUrl::fromPercentEncoding(match.captured(2).toUtf8());
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
