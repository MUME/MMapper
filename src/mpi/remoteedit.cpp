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

void RemoteEdit::slot_remoteView(const QString &title, const QString &body)
{
    addSession(REMOTE_EDIT_VIEW_KEY, title, body);
}

void RemoteEdit::slot_remoteEdit(const int key, const QString &title, const QString &body)
{
    addSession(key, title, body);
}

void RemoteEdit::addSession(const uint key, const QString &title, QString body)
{
    uint sessionId = getSessionCount();
    std::unique_ptr<RemoteEditSession> session;

    if (getConfig().mumeClientProtocol.internalRemoteEditor) {
        session = std::make_unique<RemoteEditInternalSession>(sessionId, key, title, body, this);
    } else {
        session = std::make_unique<RemoteEditExternalSession>(sessionId, key, title, body, this);
    }
    m_sessions.insert(std::make_pair(sessionId, std::move(session)));

    greatestUsedId = sessionId; // Increment sessionId counter
}

void RemoteEdit::removeSession(const uint sessionId)
{
    auto search = m_sessions.find(sessionId);
    if (search != m_sessions.end()) {
        qDebug() << "Destroying RemoteEditSession" << sessionId;
        m_sessions.erase(search);

    } else {
        qWarning() << "Unable to find" << sessionId << "session to erase";
    }
}

void RemoteEdit::cancel(const RemoteEditSession *session)
{
    assert(session != nullptr);
    if (session->isEditSession() && session->isConnected()) {
        const QString &keystr = QString("C%1\n").arg(session->getKey());
        const QByteArray &buffer
            = QString("%1E%2\n%3").arg("~$#E").arg(keystr.length()).arg(keystr).toLatin1();

        qDebug() << "Cancelling session" << session->getKey();
        emit sig_sendToSocket(buffer);
    }

    auto sessionId = session->getId();
    removeSession(sessionId);
}

void RemoteEdit::save(const RemoteEditSession *session)
{
    assert(session != nullptr);
    if (!session->isEditSession()) {
        qWarning() << "Session" << session->getId()
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

        const QString &keystr = QString("E%1\n").arg(session->getKey());
        const QByteArray &buffer = QString("%1E%2\n%3%4")
                                       .arg("~$#E")
                                       .arg(content.length() + keystr.length())
                                       .arg(keystr)
                                       .arg(content)
                                       .toLatin1();

        // MPI is always Latin1
        qDebug() << "Saving session" << session->getKey();
        emit sig_sendToSocket(buffer);

        removeSession(session->getId());
        return;
    }

    // Prompt the user to save the file somewhere since we disconnected
    const QString name
        = QFileDialog::getSaveFileName(checked_dynamic_downcast<QWidget *>(parent()), // MainWindow
                                       "MUME disconnected and you need to save the file locally",
                                       getConfig().autoLoad.lastMapDirectory + QDir::separator()
                                           + QString("MMapper-Edit-%1.txt").arg(session->getKey()),
                                       "Text files (*.txt)");
    QFile file(name);
    if (!name.isEmpty() && file.open(QFile::WriteOnly | QFile::Text)) {
        qDebug() << "Session" << session->getId() << "was was saved to" << name;
        file.write(session->getContent().toLocal8Bit());
        file.close();
    } else {
        QGuiApplication::clipboard()->setText(session->getContent());
        qWarning() << "Session" << session->getId() << "was copied to the clipboard";
    }
    removeSession(session->getId());
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
