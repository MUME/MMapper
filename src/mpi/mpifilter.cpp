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

using char_consts::C_NEWLINE;

NODISCARD static bool endsInLinefeed(const TelnetDataEnum type)
{
    switch (type) {
    case TelnetDataEnum::LF:
    case TelnetDataEnum::CRLF:
        return true;
    case TelnetDataEnum::Prompt:
    case TelnetDataEnum::Delay:
    case TelnetDataEnum::Unknown:
    default:
        return false;
    }
}

void MpiFilter::parseNewMudInput(const TelnetData &data)
{
    emit sig_parseNewMudInput(data);
}

void MpiFilter::slot_analyzeNewMudInput(const TelnetData &data)
{
    if (m_receivingMpi) {
        if (data.line.length() <= m_remaining) {
            m_buffer.append(data.line.getQByteArray());
            m_remaining -= data.line.length();
        } else {
            const QByteArray temp = data.line.getQByteArray().left(m_remaining);
            m_buffer.append(temp);
            m_remaining -= temp.length();

            // NOTE: There's an implicit assumption here is that the line cannot any text after a newline

            TelnetData remainingData;
            remainingData.type = data.type;
            remainingData.line = RawBytes{data.line.getQByteArray().right(m_remaining)};
            parseNewMudInput(remainingData);
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
                m_remaining = data.line.getQByteArray().mid(5).simplified().toInt();
                if (getConfig().mumeClientProtocol.remoteEditing
                    && (m_command == 'V' || m_command == 'E')) {
                    m_receivingMpi = true;
                }
            }
        }
        if (!m_receivingMpi) {
            parseNewMudInput(data);
        }
    }

    m_previousType = data.type;
}

void MpiFilter::parseMessage(const char command, const RawBytes &buffer)
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

void MpiFilter::parseEditMessage(const RawBytes &buffer)
{
    if (buffer.at(0) != 'M') {
        qWarning() << "Expected 'M' character in remote editing protocol";
        return;
    }

    const QByteArray &array = buffer.getQByteArray();
    int sessionEnd = array.indexOf(C_NEWLINE, 1);
    if (sessionEnd == -1) {
        qWarning() << "Unable to detect remote editing session end";
        return;
    }
    const RemoteSession sessionId = RemoteSession{array.mid(1, sessionEnd - 1)};
    int descriptionEnd = array.indexOf(C_NEWLINE, sessionEnd + 1);
    if (descriptionEnd == -1) {
        qWarning() << "Unable to detect remote editing description end";
        return;
    }
    QByteArray description = array.mid(sessionEnd + 1, (descriptionEnd - sessionEnd - 1));
    QByteArray payload = array.mid(descriptionEnd + 1);

    QString title = QString::fromLatin1(description); // MPI is always Latin1
    QString body = QString::fromLatin1(payload);      // MPI is always Latin1

    qDebug() << "Edit" << sessionId << title << "body.length=" << body.length();
    emit sig_editMessage(sessionId, title, body);
}

void MpiFilter::parseViewMessage(const RawBytes &buffer)
{
    const QByteArray &array = buffer.getQByteArray();
    int descriptionEnd = array.indexOf(C_NEWLINE);
    if (descriptionEnd == -1) {
        qWarning() << "Unable to detect remote viewing description end";
        return;
    }

    QByteArray description = array.mid(0, descriptionEnd);
    QByteArray payload = array.mid(descriptionEnd + 1);

    QString title = QString::fromLatin1(description); // MPI is always Latin1
    QString body = QString::fromLatin1(payload);      // MPI is always Latin1

    qDebug() << "Message" << title << "body.length=" << body.length();
    emit sig_viewMessage(title, body);
}
