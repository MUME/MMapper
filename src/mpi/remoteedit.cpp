// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "remoteedit.h"

#include <cassert>
#include <memory>
#include <type_traits>
#include <utility>
#include <QClipboard>
#include <QFileDialog>
#include <QGuiApplication>
#include <QMessageLogContext>
#include <QString>

#include "../configuration/configuration.h"
#include "../global/TextUtils.h"
#include "remoteeditsession.h"

RemoteEdit::RemoteEdit(QObject *const parent)
    : QObject(parent)
{
    qRegisterMetaType<RemoteSession>("RemoteSession");
}

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

    greatestUsedId = internalId; // Increment internalId counter
}

void RemoteEdit::removeSession(const RemoteEditSession *const session)
{
    const uint internalId = session->getInternalId();
    auto search = m_sessions.find(internalId);
    if (search != m_sessions.end()) {
        qDebug() << "Destroying RemoteEditSession" << internalId;
        m_sessions.erase(search);

    } else {
        qWarning() << "Unable to find" << internalId << "session to erase";
    }
}

void RemoteEdit::cancel(const RemoteEditSession *const session)
{
    assert(session != nullptr);
    if (session->isEditSession() && session->isConnected()) {
        const QString &sessionIdstr = QString("C%1\n").arg(session->getSessionId().toQString());
        const QByteArray &buffer = QString("%1E%2\n%3")
                                       .arg("~$#E")
                                       .arg(sessionIdstr.length())
                                       .arg(sessionIdstr)
                                       .toLatin1();

        qDebug() << "Cancelling session" << session->getSessionId().toQByteArray();
        emit sig_sendToSocket(buffer);
    }

    removeSession(session);
}

void RemoteEdit::save(const RemoteEditSession *const session)
{
    assert(session != nullptr);
    if (!session->isEditSession()) {
        qWarning() << "Session" << session->getInternalId()
                   << "was not an edit session and could not be saved";
        assert(false);
        return;
    }

    // Submit the edit session if we are still connected
    if (session->isConnected()) {
        // The body contents have to be followed by a LF if they are not empty
        QString content = session->getContent();
        if (!content.isEmpty() && !content.endsWith(C_NEWLINE)) {
            content.append(C_NEWLINE);
        }

        const QString &sessionIdstr = QString("E%1\n").arg(session->getSessionId().toQString());
        const QByteArray &buffer = QString("%1E%2\n%3%4")
                                       .arg("~$#E")
                                       .arg(content.length() + sessionIdstr.length())
                                       .arg(sessionIdstr)
                                       .arg(content)
                                       .toLatin1();

        // MPI is always Latin1
        qDebug() << "Saving session" << session->getSessionId().toQString();
        emit sig_sendToSocket(buffer);

        removeSession(session);
        return;
    }

    // Prompt the user to save the file somewhere since we disconnected
    const QString name = QFileDialog::getSaveFileName(
        checked_dynamic_downcast<QWidget *>(parent()), // MainWindow
        "MUME disconnected and you need to save the file locally",
        getConfig().autoLoad.lastMapDirectory + QDir::separator()
            + QString("MMapper-Edit-%1.txt").arg(session->getSessionId().toQString()),
        "Text files (*.txt)");
    QFile file(name);
    if (!name.isEmpty() && file.open(QFile::WriteOnly | QFile::Text)) {
        qDebug() << "Session" << session->getInternalId() << "was was saved to" << name;
        file.write(session->getContent().toLocal8Bit());
        file.close();
    } else {
        QGuiApplication::clipboard()->setText(session->getContent());
        qWarning() << "Session" << session->getInternalId() << "was copied to the clipboard";
    }
    removeSession(session);
}

void RemoteEdit::slot_onDisconnected()
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
