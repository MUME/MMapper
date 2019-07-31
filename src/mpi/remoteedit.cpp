// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "remoteedit.h"

#include <cassert>
#include <memory>
#include <type_traits>
#include <utility>
#include <QMessageLogContext>
#include <QRegularExpression>
#include <QString>

#include "../configuration/configuration.h"
#include "remoteeditsession.h"

static const QRegularExpression s_lineFeedNewlineRx(R"((?!\r)\n)");

void RemoteEdit::remoteView(const QString &title, const QString &body)
{
    addSession(REMOTE_EDIT_VIEW_KEY, title, body);
}

void RemoteEdit::remoteEdit(const int key, const QString &title, const QString &body)
{
    addSession(key, title, body);
}

void RemoteEdit::addSession(const int key, const QString &title, QString body)
{
    if constexpr (CURRENT_PLATFORM == PlatformEnum::Windows)
        body.replace(s_lineFeedNewlineRx, "\r\n");

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
    if (session->isEditSession()) {
        const QString &keystr = QString("C%1\n").arg(session->getKey());
        const QByteArray &buffer
            = QString("%1E%2\n%3").arg("~$#E").arg(keystr.length()).arg(keystr).toLatin1();

        qDebug() << "Cancelling session" << session->getKey() << buffer;
        emit sendToSocket(buffer);
    }

    auto sessionId = session->getId();
    removeSession(sessionId);
}

void RemoteEdit::save(const RemoteEditSession *session)
{
    assert(session != nullptr);
    if (session->isEditSession()) {
        QString content = session->getContent();
        if constexpr (CURRENT_PLATFORM == PlatformEnum::Windows)
            content.replace(s_lineFeedNewlineRx, "\n");
        // The body contents have to be followed by a LF if they are not empty
        if (!content.isEmpty() && !content.endsWith('\n')) {
            content.append('\n');
        }

        const QString &keystr = QString("E%1\n").arg(session->getKey());
        const QByteArray &buffer = QString("%1E%2\n%3%4")
                                       .arg("~$#E")
                                       .arg(content.length() + keystr.length())
                                       .arg(keystr)
                                       .arg(content)
                                       .toLatin1();

        // MPI is always Latin1
        qDebug() << "Saving session" << session->getKey() << buffer;
        emit sendToSocket(buffer);
    } else {
        qWarning() << "Session" << session->getId()
                   << "was not an edit session and could not be saved";
    }

    removeSession(session->getId());
}
