/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor),
**            Nils Schimmelmann <nschimme@gmail.com> (Jahara)
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

#include "telnetfilter.h"

#include <cstdint>
#include <QByteArray>
#include <QObject>

#include "../configuration/configuration.h"
#include "../parser/patterns.h"

static constexpr const uint8_t TC_SE = 240;  //End of subnegotiation parameters.
static constexpr const uint8_t TC_NOP = 241; //No operation
static constexpr const uint8_t TC_DM
    = 242; //Data mark. Indicates the position of a Synch event within the data stream. This should always be accompanied by a TCP urgent notification.
static constexpr const uint8_t TC_BRK
    = 243; //Break. Indicates that the "break" or "attention" key was hit.
static constexpr const uint8_t TC_IP
    = 244; //Suspend, interrupt or abort the process to which the NVT is connected.
static constexpr const uint8_t TC_AO
    = 245; //Abort output. Allows the current process to run to completion but do not send its output to the user.
static constexpr const uint8_t TC_AYT
    = 246; //Are you there. Send back to the NVT some visible evidence that the AYT was received.
static constexpr const uint8_t TC_EC
    = 247; //Erase character. The receiver should delete the last preceding undeleted character from the data stream.
static constexpr const uint8_t TC_EL
    = 248; //Erase line. Delete characters from the data stream back to but not including the previous CRLF.
static constexpr const uint8_t TC_GA
    = 249; //Go ahead. Used, under certain circumstances, to tell the other end that it can transmit.
static constexpr const uint8_t TC_SB = 250; //Subnegotiation of the indicated option follows.
static constexpr const uint8_t TC_WILL
    = 251; //Indicates the desire to begin performing, or confirmation that you are now performing, the indicated option.
static constexpr const uint8_t TC_WONT
    = 252; //Indicates the refusal to perform, or continue performing, the indicated option.
static constexpr const uint8_t TC_DO
    = 253; //Indicates the request that the other party perform, or confirmation that you are expecting the other party to perform, the indicated option.
static constexpr const uint8_t TC_DONT
    = 254; //Indicates the demand that the other party stop performing, or confirmation that you are no longer expecting the other party to perform, the indicated option.
static constexpr const uint8_t TC_IAC = 255;
static constexpr const uint8_t ASCII_DEL = 8;
static constexpr const uint8_t ASCII_CR = 13;
static constexpr const uint8_t ASCII_LF = 10;
// static constexpr const uint8_t ESCAPE = '\x1B';

TelnetFilter::TelnetFilter(QObject *parent)
    : QObject(parent)
{
#ifdef TELNET_STREAM_DEBUG_INPUT_TO_FILE
    QString fileName = "telnet_debug.dat";

    file = new QFile(fileName);

    if (!file->open(QFile::WriteOnly))
        return;

    debugStream = new QDataStream(file);
#endif
}

TelnetFilter::~TelnetFilter()
{
#ifdef TELNET_STREAM_DEBUG_INPUT_TO_FILE
    file->close();
#endif
}

void TelnetFilter::analyzeMudStream(const QByteArray &ba)
{
    dispatchTelnetStream(ba, m_mudIncomingData, m_mudIncomingQue);
    IncomingData data;
    while (!m_mudIncomingQue.isEmpty()) {
        data = m_mudIncomingQue.dequeue();
        //parse incoming lines in que
        emit parseNewMudInput(data /*m_mudIncomingQue*/);
    }
}

void TelnetFilter::analyzeUserStream(const QByteArray &ba)
{
    dispatchTelnetStream(ba, m_userIncomingData, m_userIncomingQue);

    //parse incoming lines in que
    IncomingData data;
    while (!m_userIncomingQue.isEmpty()) {
        data = m_userIncomingQue.dequeue();

        emit parseNewUserInput(data);
    }
}

void TelnetFilter::dispatchTelnetStream(const QByteArray &stream,
                                        IncomingData &m_incomingData,
                                        TelnetIncomingDataQueue &que)
{
    quint16 index = 0; //Here br

#ifdef TELNET_STREAM_DEBUG_INPUT_TO_FILE
    if (stream.contains("<")) {
        (*debugStream) << "***S***";
        (*debugStream) << stream;
        (*debugStream) << "***E***";
    }
#endif

    /* REVISIT: This code doesn't look like it robustly handles IAC SB ... IAC SE,
     * since SB is never used in this file. */
    while (index < stream.size()) {
        const auto val1 = static_cast<quint8>(stream.at(index));
        switch (val1) {
        case ASCII_DEL:
            m_incomingData.line.append(static_cast<char>(ASCII_DEL));

            if (m_incomingData.type != TelnetDataType::TELNET) {
                m_incomingData.type = TelnetDataType::DELAY;
                que.enqueue(m_incomingData);
                m_incomingData.line.clear();
                m_incomingData.type = TelnetDataType::SPLIT;
            }
            index++;
            break;

        case TC_IAC: //IAC IAC  - should not happen tho, we use ASCII chars only!!!

            if (m_incomingData.type != TelnetDataType::TELNET && !m_incomingData.line.isEmpty()) {
                if (Config().mumeClientProtocol.IAC_prompt_parser && index > 0
                    && (static_cast<quint8>(stream.at(index - 1)))
                           == '>' /*&& ((quint8) stream.at(index+1)) == TC_GA*/) {
                    //prompt
                    m_incomingData.type = TelnetDataType::PROMPT;
                } else {
                    m_incomingData.type = TelnetDataType::UNKNOWN;
                }
                que.enqueue(m_incomingData);
                m_incomingData.line.clear();
            }
            //start incoming telnet command
            m_incomingData.type = TelnetDataType::TELNET;
            m_incomingData.line.append(static_cast<char>(val1));
            index++;
            break;

        case TC_GA:
        case TC_NOP:
        case TC_DM:
        case TC_BRK:
        case TC_IP:
        case TC_AO:
        case TC_AYT:
        case TC_EC:
        case TC_EL:
        case TC_SE:
            if (m_incomingData.type == TelnetDataType::TELNET) {
                //end incoming telnet command
                m_incomingData.line.append(static_cast<char>(val1));
                que.enqueue(m_incomingData);
                m_incomingData.line.clear();
                if (false && val1 == TC_GA && Config().mumeClientProtocol.IAC_prompt_parser)
                    m_incomingData.type = TelnetDataType::PROMPT;
                else
                    m_incomingData.type = TelnetDataType::SPLIT;

            } else {
                m_incomingData.line.append(static_cast<char>(val1));
            }
            index++;
            break;

        case ASCII_CR:
            if (!m_incomingData.line.isEmpty() && m_incomingData.type != TelnetDataType::TELNET) {
                const auto val2 = m_incomingData.line.right(1).at(0); //get last char
                switch (val2) {
                case ASCII_LF:
                    m_incomingData.line.append(static_cast<char>(ASCII_CR));
                    m_incomingData.type = TelnetDataType::LFCR;
                    {
                        que.enqueue(m_incomingData);
                    }
                    m_incomingData.line.clear();
                    m_incomingData.type = TelnetDataType::SPLIT;
                    index++;
                    break;

                default:
                    m_incomingData.line.append(static_cast<char>(ASCII_CR));
                    index++;
                    break;
                }
            } else {
                m_incomingData.line.append(static_cast<char>(ASCII_CR));
                index++;
                break;
            }
            break;

        case ASCII_LF:
            if (!m_incomingData.line.isEmpty() && m_incomingData.type != TelnetDataType::TELNET) {
                const auto val2 = static_cast<uint8_t>(
                    m_incomingData.line.right(1).at(0)); //get last char
                switch (val2) {
                case ASCII_CR:
                    m_incomingData.line.append(static_cast<char>(ASCII_LF));
                    m_incomingData.type = TelnetDataType::CRLF;
                    {
                        que.enqueue(m_incomingData);
                    }
                    m_incomingData.line.clear();
                    m_incomingData.type = TelnetDataType::SPLIT;
                    index++;
                    break;

                default:
                    m_incomingData.line.append(static_cast<char>(ASCII_LF));
                    index++;
                    break;
                }
            } else {
                m_incomingData.line.append(static_cast<char>(ASCII_LF));
                index++;
                break;
            }
            break;

        default:
            if (m_incomingData.type == TelnetDataType::TELNET) {
                if (!m_incomingData.line.isEmpty()) {
                    const auto val2 = static_cast<uint8_t>(
                        m_incomingData.line.right(1).at(0)); //get last char
                    switch (val2) {
                    case TC_DONT:
                    case TC_DO:
                    case TC_WONT:
                    case TC_WILL:
                        //end incoming telnet command
                        m_incomingData.line.append(static_cast<char>(val1));
                        {
                            que.enqueue(m_incomingData);
                        }
                        m_incomingData.line.clear();
                        m_incomingData.type = TelnetDataType::SPLIT;
                        index++;
                        break;

                    default:
                        m_incomingData.line.append(static_cast<char>(val1));
                        index++;
                        break;
                    }
                }
            } else {
                if (!m_incomingData.line.isEmpty()
                    && m_incomingData.line.right(1).at(0) == ASCII_LF) {
                    m_incomingData.type = TelnetDataType::LF;
                    que.enqueue(m_incomingData);
                    m_incomingData.line.clear();
                    m_incomingData.type = TelnetDataType::SPLIT;
                }

                m_incomingData.line.append(static_cast<char>(val1));
                index++;
                break;
            }
            break;
        }
    }

    if (!m_incomingData.line.isEmpty() && m_incomingData.type == TelnetDataType::SPLIT) {
        {
            if (m_incomingData.line.endsWith("prompt>")) {
                m_incomingData.type = TelnetDataType::PROMPT;
                que.enqueue(m_incomingData);
                m_incomingData.line.clear();
                m_incomingData.type = TelnetDataType::SPLIT;
            } else if (m_incomingData.line.endsWith(char(ASCII_LF))) {
                m_incomingData.type = TelnetDataType::LF;
                que.enqueue(m_incomingData);
                m_incomingData.line.clear();
                m_incomingData.type = TelnetDataType::SPLIT;
            } else if (Patterns::matchPromptPatterns(m_incomingData.line)) {
                m_incomingData.type = TelnetDataType::PROMPT;
                que.enqueue(m_incomingData);
                m_incomingData.line.clear();
                m_incomingData.type = TelnetDataType::SPLIT;
            } else if (Patterns::matchPasswordPatterns(m_incomingData.line)) {
                m_incomingData.type = TelnetDataType::LOGIN_PASSWORD;
                que.enqueue(m_incomingData);
                m_incomingData.line.clear();
                m_incomingData.type = TelnetDataType::SPLIT;
            } else if (Patterns::matchMenuPromptPatterns(m_incomingData.line)) {
                m_incomingData.type = TelnetDataType::MENU_PROMPT;
                que.enqueue(m_incomingData);
                m_incomingData.line.clear();
                m_incomingData.type = TelnetDataType::SPLIT;
            } else if (Patterns::matchLoginPatterns(m_incomingData.line)) {
                m_incomingData.type = TelnetDataType::LOGIN;
                que.enqueue(m_incomingData);
                m_incomingData.line.clear();
                m_incomingData.type = TelnetDataType::SPLIT;
            }
        }
    }
}
