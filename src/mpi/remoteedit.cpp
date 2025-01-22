// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "remoteedit.h"

#include "../configuration/configuration.h"
#include "../global/Consts.h"
#include "remoteeditsession.h"

#include <cassert>
#include <memory>
#include <type_traits>
#include <utility>

#include <QClipboard>
#include <QFileDialog>
#include <QGuiApplication>
#include <QMessageLogContext>
#include <QString>

using char_consts::C_NEWLINE;

RemoteEdit::RemoteEdit(QObject *const parent)
    : QObject(parent)
{}

void RemoteEdit::slot_remoteView(const QString &title, const QString &body)
{
    addSession(REMOTE_VIEW_SESSION_ID, title, body);
}

void RemoteEdit::slot_remoteEdit(const RemoteSession &sessionId,
                                 const QString &title,
                                 const QString &body)
{
    addSession(sessionId, title, body);
}

void RemoteEdit::addSession(const RemoteSession &sessionId,
                            const QString &title,
                            const QString &body)
{
    const auto internalId = getInternalIdCount();
    std::unique_ptr<RemoteEditSession> session;

    if (getConfig().mumeClientProtocol.internalRemoteEditor) {
        session = std::make_unique<RemoteEditInternalSession>(internalId,
                                                              sessionId,
                                                              title,
                                                              body,
                                                              this);
    } else {
        session = std::make_unique<RemoteEditExternalSession>(internalId,
                                                              sessionId,
                                                              title,
                                                              body,
                                                              this);
    }
    m_sessions.insert(std::make_pair(internalId, std::move(session)));

    m_greatestUsedId = internalId; // Increment internalId counter
}

void RemoteEdit::removeSession(const RemoteEditSession &session)
{
    const uint32_t internalId = session.getInternalId();
    const auto search = m_sessions.find(internalId);
    if (search != m_sessions.end()) {
        qDebug() << "Destroying RemoteEditSession" << internalId;
        m_sessions.erase(search);

    } else {
        qWarning() << "Unable to find" << internalId << "session to erase";
    }
}

void RemoteEdit::cancel(const RemoteEditSession *const pSession)
{
    auto &session = deref(pSession);

    if (session.isEditSession() && session.isConnected()) {
        qDebug() << "Cancelling session" << session.getSessionId();
        emit sig_remoteEditCancel(session.getSessionId());
    }

    removeSession(session);
}

void RemoteEdit::save(const RemoteEditSession *const pSession)
{
    auto &session = deref(pSession);
    trySave(session);
    removeSession(session);
}

void RemoteEdit::trySave(const RemoteEditSession &session)
{
    if (!session.isEditSession()) {
        qWarning() << "Session" << session.getInternalId()
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

    qDebug() << "Saving session" << session.getSessionId();
    // REVISIT: should we warn if this transformation modifies the content
    // (e.g. unicode transliteration, etc).
    auto latin1 = Latin1Bytes{
        mmqt::toQByteArrayLatin1(session.getContent())}; // MPI is always Latin1
    emit sig_remoteEditSave(session.getSessionId(), latin1);
}

void RemoteEdit::trySaveLocally(const RemoteEditSession &session)
{
    if (!session.isEditSession()) {
        std::abort();
    }

    // Prompt the user to save the file somewhere since we disconnected
    const QString name = QFileDialog::getSaveFileName(
        checked_dynamic_downcast<QWidget *>(parent()), // MainWindow
        "MUME disconnected and you need to save the file locally",
        getConfig().autoLoad.lastMapDirectory + QDir::separator()
            + QString("MMapper-Edit-%1.txt").arg(session.getSessionId().getQByteArray().constData()),
        "Text files (*.txt)");

    QFile file(name);
    if (!name.isEmpty() && file.open(QFile::WriteOnly | QFile::Text)) {
        qDebug() << "Session" << session.getInternalId() << "was was saved to" << name;
        file.write(session.getContent().toUtf8());
        file.close();
    } else {
        QGuiApplication::clipboard()->setText(session.getContent());
        qWarning() << "Session" << session.getInternalId() << "was copied to the clipboard";
    }
}

void RemoteEdit::onDisconnected()
{
    for (const auto &pair : m_sessions) {
        const auto &id = pair.first;
        const auto &session = pair.second;
        if (session->isEditSession()) {
            qWarning() << "Session" << id << "marked as disconnected";
            session->setDisconnected();
        }
    }
}
