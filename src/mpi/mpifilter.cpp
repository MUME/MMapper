// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "mpifilter.h"

#include "../configuration/configuration.h"
#include "../proxy/telnetfilter.h"

#include <QByteArray>
#include <QMessageLogContext>
#include <QObject>
#include <QString>

NODISCARD static bool endsInLinefeed(const TelnetDataEnum type)
{
    switch (type) {
    case TelnetDataEnum::LF:
    case TelnetDataEnum::CRLF:
        return true;
    case TelnetDataEnum::PROMPT:
    case TelnetDataEnum::DELAY:
    case TelnetDataEnum::UNKNOWN:
    default:
        return false;
    }
}

void MpiFilter::slot_analyzeNewMudInput(const TelnetData &data)
{
    if (m_receivingMpi) {
        if (data.line.length() <= m_remaining) {
            m_buffer.append(data.line);
            m_remaining -= data.line.length();
        } else {
            const QByteArray temp = data.line.left(m_remaining);
            m_buffer.append(temp);
            m_remaining -= temp.length();

            TelnetData remainingData;
            remainingData.type = data.type;
            remainingData.line = data.line.right(m_remaining);
            emit sig_parseNewMudInput(remainingData);
        }
        if (m_remaining == 0) {
            m_receivingMpi = false;
            parseMessage(m_command, m_buffer);
        }

    } else {
        // mume protocol spec requires LF before start of MPI message
        if (endsInLinefeed(m_previousType)) {
            if (!m_receivingMpi && data.line.length() >= 6 && data.line.startsWith("~$#E")) {
                m_buffer.clear();
                m_command = data.line.at(4);
                m_remaining = data.line.mid(5).simplified().toInt();
                if (getConfig().mumeClientProtocol.remoteEditing
                    && (m_command == 'V' || m_command == 'E')) {
                    m_receivingMpi = true;
                }
            }
        }
        if (!m_receivingMpi) {
            emit sig_parseNewMudInput(data);
        }
    }

    m_previousType = data.type;
}

void MpiFilter::parseMessage(const char command, const QByteArray &buffer)
{
    switch (command) {
    case 'E':
        parseEditMessage(buffer);
        break;
    case 'V':
        parseViewMessage(buffer);
        break;
    default:
        // Unknown command
        qWarning() << "Unsupported remote editing message command" << command;
        break;
    }
}

void MpiFilter::parseEditMessage(const QByteArray &buffer)
{
    if (buffer.at(0) != 'M') {
        qWarning() << "Expected 'M' character in remote editing protocol";
        return;
    }
    int sessionEnd = buffer.indexOf('\n', 1);
    if (sessionEnd == -1) {
        qWarning() << "Unable to detect remote editing session end";
        return;
    }
    const RemoteSession sessionId = RemoteSession(buffer.mid(1, sessionEnd - 1).toStdString());
    int descriptionEnd = buffer.indexOf('\n', sessionEnd + 1);
    if (descriptionEnd == -1) {
        qWarning() << "Unable to detect remote editing description end";
        return;
    }
    QByteArray description = buffer.mid(sessionEnd + 1, (descriptionEnd - sessionEnd - 1));
    QByteArray payload = buffer.mid(descriptionEnd + 1);

    // MPI is always Latin1
    QString title = QString::fromLatin1(description);
    QString body = QString::fromLatin1(payload);

    qDebug() << "Edit" << sessionId.toQString() << title << "body.length=" << body.length();
    emit sig_editMessage(sessionId, title, body);
}

void MpiFilter::parseViewMessage(const QByteArray &buffer)
{
    int descriptionEnd = buffer.indexOf('\n');
    if (descriptionEnd == -1) {
        qWarning() << "Unable to detect remote viewing description end";
        return;
    }
    QByteArray description = buffer.mid(0, descriptionEnd);
    QByteArray payload = buffer.mid(descriptionEnd + 1);

    // MPI is always Latin1
    QString title = QString::fromLatin1(description);
    QString body = QString::fromLatin1(payload);

    qDebug() << "Message" << title << "body.length=" << body.length();
    emit sig_viewMessage(title, body);
}
