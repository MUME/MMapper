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

#include "../parser/patterns.h"

static constexpr const uint8_t ASCII_DEL = 8;
static constexpr const uint8_t ASCII_CR = 13;
static constexpr const uint8_t ASCII_LF = 10;

static IncomingData createIncomingDataPrompt(const QByteArray &data)
{
    IncomingData incomingData;
    incomingData.line = data;
    incomingData.type = TelnetDataType::PROMPT;
    return incomingData;
}

TelnetFilter::TelnetFilter(QObject *parent)
    : QObject(parent)
{}

void TelnetFilter::onAnalyzeMudStream(const QByteArray &ba, bool goAhead)
{
    if (goAhead) {
        emit parseNewMudInput(createIncomingDataPrompt(ba));
        return;
    }
    dispatchTelnetStream(ba, m_mudIncomingBuffer, m_mudIncomingQue);

    //parse incoming lines in que
    IncomingData data;
    while (!m_mudIncomingQue.isEmpty()) {
        data = m_mudIncomingQue.dequeue();
        emit parseNewMudInput(data);
    }
}

void TelnetFilter::onAnalyzeUserStream(const QByteArray &ba, bool goAhead)
{
    if (goAhead) {
        emit parseNewMudInput(createIncomingDataPrompt(ba));
        return;
    }
    dispatchTelnetStream(ba, m_userIncomingData, m_userIncomingQue);

    //parse incoming lines in que
    IncomingData data;
    while (!m_userIncomingQue.isEmpty()) {
        data = m_userIncomingQue.dequeue();
        emit parseNewUserInput(data);
    }
}

void TelnetFilter::dispatchTelnetStream(const QByteArray &stream,
                                        IncomingData &buffer,
                                        TelnetIncomingDataQueue &que)
{
    quint16 index = 0;

    while (index < stream.size()) {
        const auto val1 = static_cast<uint8_t>(stream.at(index));
        switch (val1) {
        case ASCII_DEL:
            buffer.line.append(static_cast<char>(ASCII_DEL));

            buffer.type = TelnetDataType::DELAY;
            que.enqueue(buffer);
            buffer.line.clear();
            buffer.type = TelnetDataType::SPLIT;
            index++;
            break;

        case ASCII_CR:
            if (!buffer.line.isEmpty()) {
                const auto val2 = buffer.line.right(1).at(0); //get last char
                switch (val2) {
                case ASCII_LF:
                    buffer.line.append(static_cast<char>(ASCII_CR));
                    buffer.type = TelnetDataType::LFCR;
                    {
                        que.enqueue(buffer);
                    }
                    buffer.line.clear();
                    buffer.type = TelnetDataType::SPLIT;
                    index++;
                    break;

                default:
                    buffer.line.append(static_cast<char>(ASCII_CR));
                    index++;
                    break;
                }
            } else {
                buffer.line.append(static_cast<char>(ASCII_CR));
                index++;
                break;
            }
            break;

        case ASCII_LF:
            if (!buffer.line.isEmpty()) {
                const auto val2 = static_cast<uint8_t>(buffer.line.right(1).at(0)); //get last char
                switch (val2) {
                case ASCII_CR:
                    buffer.line.append(static_cast<char>(ASCII_LF));
                    buffer.type = TelnetDataType::CRLF;
                    {
                        que.enqueue(buffer);
                    }
                    buffer.line.clear();
                    buffer.type = TelnetDataType::SPLIT;
                    index++;
                    break;

                default:
                    buffer.line.append(static_cast<char>(ASCII_LF));
                    index++;
                    break;
                }
            } else {
                buffer.line.append(static_cast<char>(ASCII_LF));
                index++;
                break;
            }
            break;

        default:
            if (!buffer.line.isEmpty() && buffer.line.right(1).at(0) == ASCII_LF) {
                buffer.type = TelnetDataType::LF;
                que.enqueue(buffer);
                buffer.line.clear();
                buffer.type = TelnetDataType::SPLIT;
            }

            buffer.line.append(static_cast<char>(val1));
            index++;
            break;
        }
    }

    if (!buffer.line.isEmpty() && buffer.type == TelnetDataType::SPLIT) {
        {
            if (buffer.line.endsWith(char(ASCII_LF))) {
                buffer.type = TelnetDataType::LF;
                que.enqueue(buffer);
                buffer.line.clear();
                buffer.type = TelnetDataType::SPLIT;
            } else if (Patterns::matchPromptPatterns(buffer.line)) {
                buffer.type = TelnetDataType::PROMPT;
                que.enqueue(buffer);
                buffer.line.clear();
                buffer.type = TelnetDataType::SPLIT;
            } else if (Patterns::matchPasswordPatterns(buffer.line)) {
                buffer.type = TelnetDataType::LOGIN_PASSWORD;
                que.enqueue(buffer);
                buffer.line.clear();
                buffer.type = TelnetDataType::SPLIT;
            } else if (Patterns::matchMenuPromptPatterns(buffer.line)) {
                buffer.type = TelnetDataType::MENU_PROMPT;
                que.enqueue(buffer);
                buffer.line.clear();
                buffer.type = TelnetDataType::SPLIT;
            } else if (Patterns::matchLoginPatterns(buffer.line)) {
                buffer.type = TelnetDataType::LOGIN;
                que.enqueue(buffer);
                buffer.line.clear();
                buffer.type = TelnetDataType::SPLIT;
            }
        }
    }
}
