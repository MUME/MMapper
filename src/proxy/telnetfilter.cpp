// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "telnetfilter.h"

#include <cstdint>
#include <QByteArray>
#include <QObject>

#include "../parser/patterns.h"

static constexpr const uint8_t ASCII_DEL = 8;
static constexpr const uint8_t ASCII_CR = 13;
static constexpr const uint8_t ASCII_LF = 10;

void TelnetFilter::slot_onAnalyzeMudStream(const QByteArray &ba, bool goAhead)
{
    dispatchTelnetStream(ba, m_mudIncomingBuffer, m_mudIncomingQue, goAhead);

    // parse incoming lines in que
    TelnetData data;
    while (!m_mudIncomingQue.isEmpty()) {
        data = m_mudIncomingQue.dequeue();
        emit sig_parseNewMudInput(data);
    }
}

void TelnetFilter::slot_onAnalyzeUserStream(const QByteArray &ba, bool goAhead)
{
    dispatchTelnetStream(ba, m_userIncomingData, m_userIncomingQue, goAhead);

    // parse incoming lines in que
    TelnetData data;
    while (!m_userIncomingQue.isEmpty()) {
        data = m_userIncomingQue.dequeue();
        emit sig_parseNewUserInput(data);
    }
}

void TelnetFilter::dispatchTelnetStream(const QByteArray &stream,
                                        TelnetData &buffer,
                                        TelnetIncomingDataQueue &que,
                                        const bool &goAhead)
{
    quint16 index = 0;

    while (index < stream.size()) {
        const auto val1 = static_cast<uint8_t>(stream.at(index));
        switch (val1) {
        case ASCII_DEL:
            buffer.line.append(static_cast<char>(ASCII_DEL));
            buffer.type = TelnetDataEnum::DELAY;
            que.enqueue(buffer);
            buffer.line.clear();
            buffer.type = TelnetDataEnum::UNKNOWN;
            index++;
            break;

        case ASCII_CR:
            buffer.line.append(static_cast<char>(ASCII_CR));
            index++;
            break;

        case ASCII_LF:
            if (!buffer.line.isEmpty()) {
                const auto val2 = static_cast<uint8_t>(buffer.line.right(1).at(0)); // get last char
                switch (val2) {
                case ASCII_CR:
                    buffer.line.append(static_cast<char>(ASCII_LF));
                    buffer.type = TelnetDataEnum::CRLF;
                    que.enqueue(buffer);
                    buffer.line.clear();
                    buffer.type = TelnetDataEnum::UNKNOWN;
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
                buffer.type = TelnetDataEnum::LF;
                que.enqueue(buffer);
                buffer.line.clear();
                buffer.type = TelnetDataEnum::UNKNOWN;
            }

            buffer.line.append(static_cast<char>(val1));
            index++;
            break;
        }
    }

    if (!buffer.line.isEmpty() && (goAhead || buffer.type == TelnetDataEnum::UNKNOWN)) {
        {
            if (goAhead) {
                const auto get_type = [&buffer]() {
                    if (Patterns::matchPasswordPatterns(buffer.line))
                        return TelnetDataEnum::LOGIN_PASSWORD;
                    else if (Patterns::matchMenuPromptPatterns(buffer.line))
                        return TelnetDataEnum::MENU_PROMPT;
                    return TelnetDataEnum::PROMPT;
                };
                buffer.type = get_type();
                que.enqueue(buffer);
                buffer.line.clear();
                buffer.type = TelnetDataEnum::UNKNOWN;
            } else if (buffer.line.endsWith(char(ASCII_LF))) {
                buffer.type = TelnetDataEnum::LF;
                que.enqueue(buffer);
                buffer.line.clear();
                buffer.type = TelnetDataEnum::UNKNOWN;
            } else if (Patterns::matchLoginPatterns(buffer.line)) {
                // IAC-GA usually take effect after the login screen
                buffer.type = TelnetDataEnum::LOGIN;
                que.enqueue(buffer);
                buffer.line.clear();
                buffer.type = TelnetDataEnum::UNKNOWN;
            }
        }
    }
}
