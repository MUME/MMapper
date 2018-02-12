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

#include "mpifilter.h"
#include "configuration/configuration.h"

#include <QDebug>

MpiFilter::MpiFilter(QObject *parent) : QObject(parent),
    m_previousType(TDT_UNKNOWN),
    m_parsingMpi(false),
    m_remaining(0)
{
}

void MpiFilter::analyzeNewMudInput(IncomingData &data)
{
    if (m_parsingMpi) {
        if (data.line.length() <= m_remaining) {
            m_buffer.append(data.line);
            m_remaining -= data.line.length();
        } else {
            QByteArray temp = data.line.left(m_remaining);
            m_buffer.append(temp);
            m_remaining -= temp.length();

            IncomingData remainingData;
            remainingData.type = data.type;
            remainingData.line = data.line.right(m_remaining);
            emit parseNewMudInput(remainingData);
        }
        if (m_remaining == 0) {
            m_parsingMpi = false;
            parseMessage(m_command, m_buffer);
        }

    } else {
        switch (m_previousType) {
        case TDT_LF:
        case TDT_CRLF:
            if (!m_parsingMpi && data.line.length() >= 6 && data.line.startsWith("~$#E")) {
                m_buffer.clear();
                m_command = data.line.at(4);
                m_remaining = data.line.mid(5).simplified().toInt();
                if (Config().m_remoteEditing && (m_command == 'V' || m_command == 'E')) {
                    m_parsingMpi = true;
                    break;
                }
            }
        default:
            emit parseNewMudInput(data);
            break;
        }
    }

    m_previousType = data.type;
}

void MpiFilter::parseMessage(char command, QByteArray buffer)
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

void MpiFilter::parseEditMessage(QByteArray buffer)
{
    if (buffer.at(0) != 'M') {
        qWarning() << "Expected 'M' character in remote editing protocol";
        return ;
    }
    int sessionEnd = buffer.indexOf('\n', 1);
    if (sessionEnd == -1) {
        qWarning() << "Unable to detect remote editing session end";
        return ;
    }
    int sessionId = buffer.mid(1, sessionEnd - 1).toInt();
    int descriptionEnd = buffer.indexOf('\n', sessionEnd + 1);
    if (descriptionEnd == -1) {
        qWarning() << "Unable to detect remote editing description end";
        return;
    }
    QByteArray description = buffer.mid(sessionEnd + 1, (descriptionEnd - sessionEnd - 1));
    QByteArray payload = buffer.mid(descriptionEnd + 1);

    // Payload is never UTF-8
    QString title = QString::fromLatin1(description);
    QString body = QString::fromLatin1(payload);

    qDebug() << "Edit" << sessionId << title << body;
    emit editMessage(sessionId, title, body);
}

void MpiFilter::parseViewMessage(QByteArray buffer)
{
    int descriptionEnd = buffer.indexOf('\n');
    if (descriptionEnd == -1) {
        qWarning() << "Unable to detect remote viewing description end";
        return;
    }
    QByteArray description = buffer.mid(0, descriptionEnd);
    QByteArray payload = buffer.mid(descriptionEnd + 1);

    // Payload is never UTF-8
    QString title = QString::fromLatin1(description);
    QString body = QString::fromLatin1(payload);

    qDebug() << "Message" << title << body;
    emit viewMessage(title, body);
}

