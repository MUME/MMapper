/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com> (Jahara)
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/
#include "remoteedit.h"
#include "configuration/configuration.h"
#include "remoteeditsession.h"

#include <cassert>
#include <QDebug>

const QRegExp RemoteEdit::s_lineFeedNewlineRx("(?!\\r)\\n");

void RemoteEdit::remoteView(const QString &title, const QString &body)
{
    addSession(REMOTE_EDIT_VIEW_KEY, title, body);
}

void RemoteEdit::remoteEdit(const int key, const QString &title, const QString &body)
{
    QString content = body;
#ifdef Q_OS_WIN
    content.replace(s_lineFeedNewlineRx, "\r\n");
#endif
    addSession(key, title, content);
}

void RemoteEdit::addSession(const int key, const QString &title, const QString &body)
{
    uint sessionId = getSessionCount();
    std::unique_ptr<RemoteEditSession> session;
    if (Config().m_internalRemoteEditor) {
        session = std::make_unique<RemoteEditInternalSession>(sessionId, key, title, body, this);
    } else {
        session = std::make_unique<RemoteEditExternalSession>(sessionId, key, title, body, this);
    }
    m_sessions.insert(std::make_pair(sessionId, std::move(session)));
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

    int sessionId = session->getId();
    removeSession(sessionId);
}

void RemoteEdit::save(const RemoteEditSession *session)
{
    assert(session != nullptr);
    if (session->isEditSession()) {
        QString content = session->getContent();
#ifdef Q_OS_WIN
        content.replace("\r\n", "\n");
#endif
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

        qDebug() << "Saving session" << session->getKey() << buffer;
        emit sendToSocket(buffer);
    } else {
        qWarning() << "Session" << session->getId()
                   << "was not an edit session and could not be saved";
    }

    removeSession(session->getId());
}
