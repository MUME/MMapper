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

static constexpr const char *const MPI_PREFIX = "~$#E";
using char_consts::C_NEWLINE;

NODISCARD static bool endsInLinefeed(const TelnetDataEnum type)
{
    switch (type) {
    case TelnetDataEnum::LF:
    case TelnetDataEnum::CRLF:
        return true;
    case TelnetDataEnum::Prompt:
    case TelnetDataEnum::Backspace:
    case TelnetDataEnum::Empty:
    default:
        return false;
    }
}

MpiFilterOutputs::~MpiFilterOutputs() = default;

void MpiFilter::analyzeNewMudInput(const TelnetData &data)
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
            m_outputs.onParseNewMudInput(remainingData);
        }
        if (m_remaining == 0) {
            m_receivingMpi = false;
            parseMessage(m_command, m_buffer);
        }

    } else {
        // mume protocol spec requires LF before start of MPI message
        if (endsInLinefeed(m_previousType)) {
            // REVISIT: static analysis says !m_receivingMpi is always true (looks correct).
            if (!m_receivingMpi && data.line.length() >= 6 && data.line.startsWith(MPI_PREFIX)) {
                m_buffer.clear();
                m_command = data.line.at(4);
                m_remaining = data.line.getQByteArray().mid(5).simplified().toInt();
                if (getConfig().mumeClientProtocol.remoteEditing
                    && (m_command == 'V' || m_command == 'E')) {
                    m_receivingMpi = true;
                }
            }
        }
        // REVISIT: static analysis says !m_receivingMpi is always true,
        // but it must not see the m_receivingMpi = true in the block above.
        if (!m_receivingMpi) {
            const volatile bool filterBareNewlines = false;
            if (filterBareNewlines && data.type == TelnetDataEnum::LF
                && data.line.getQByteArray() == "\n") {
                // this is a special case used by Mume to force MPI messages to follow a newline
                // after a prompt; this only occurs when Mume sends an MPI as the first text
                // of a command. All non-MPI messages use CRLF instead of just bare newlines.
                qInfo() << "Filtered bare newline.";
            } else {
                m_outputs.onParseNewMudInput(data);
            }
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
    m_outputs.onEditMessage(sessionId, title, body);
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
    m_outputs.onViewMessage(title, body);
}

MpiFilterToMud::~MpiFilterToMud() = default;

void MpiFilterToMud::cancelRemoteEdit(const RemoteEditMessageBytes &sessionId)
{
    const QString &sessionIdstr = QString("C%1\n").arg(sessionId.getQByteArray().constData());
    const QByteArray buffer = QString("%1E%2\n%3")
                                  .arg(MPI_PREFIX)
                                  .arg(sessionIdstr.length())
                                  .arg(sessionIdstr)
                                  .toLatin1(); // MPI requires latin1

    submitMpi(RawBytes{buffer});
}

void MpiFilterToMud::saveRemoteEdit(const RemoteEditMessageBytes &sessionId,
                                    const Latin1Bytes &content)
{
    auto payload = content.getQByteArray();

    // The body contents have to be followed by a LF if they are not empty
    if (!payload.isEmpty() && !payload.endsWith(char_consts::C_NEWLINE)) {
        payload.append(char_consts::C_NEWLINE);
    }

    const QString &sessionIdstr = QString("E%1\n").arg(sessionId.getQByteArray().constData());
    QByteArray buffer = QString("%1E%2\n%3")
                            .arg(MPI_PREFIX)
                            .arg(payload.length() + sessionIdstr.length())
                            .arg(sessionIdstr)
                            .toLatin1(); // MPI is always Latin1

    buffer += payload;

    submitMpi(RawBytes{buffer});
}
void MpiFilterToMud::submitMpi(const RawBytes &bytes)
{
    assert(isMpiMessage(bytes));
    // consider validating the content of the message here.
    virt_submitMpi(bytes);
}

bool isMpiMessage(const RawBytes &bytes)
{
    return !bytes.isEmpty() && bytes.startsWith(MPI_PREFIX) && bytes.back() == C_NEWLINE;
}

bool hasMpiPrefix(const QString &s)
{
    return s.startsWith(MPI_PREFIX);
}
