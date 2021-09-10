// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "telnetfilter.h"

#include <QByteArray>
#include <QObject>

static constexpr const char ASCII_DEL = '\x08';
static constexpr const char ASCII_CR = '\r';
static constexpr const char ASCII_LF = '\n';

static_assert(ASCII_DEL == 8);
static_assert(ASCII_LF == 10);
static_assert(ASCII_CR == 13);

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
    for (const char c : stream) {
        switch (c) {
        case ASCII_DEL:
            buffer.line.append(ASCII_DEL);
            buffer.type = TelnetDataEnum::DELAY;
            que.enqueue(buffer);
            buffer.line.clear();
            buffer.type = TelnetDataEnum::UNKNOWN;
            break;

        case ASCII_CR:
            buffer.line.append(ASCII_CR);
            break;

        case ASCII_LF:
            if (!buffer.line.isEmpty()) {
                switch (buffer.line.back()) {
                case ASCII_CR:
                    buffer.line.append(ASCII_LF);
                    buffer.type = TelnetDataEnum::CRLF;
                    que.enqueue(buffer);
                    buffer.line.clear();
                    buffer.type = TelnetDataEnum::UNKNOWN;
                    break;

                default:
                    buffer.line.append(ASCII_LF);
                    break;
                }
            } else {
                buffer.line.append(ASCII_LF);
                break;
            }
            break;

        default:
            if (!buffer.line.isEmpty() && buffer.line.back() == ASCII_LF) {
                buffer.type = TelnetDataEnum::LF;
                que.enqueue(buffer);
                buffer.line.clear();
                buffer.type = TelnetDataEnum::UNKNOWN;
            }

            buffer.line.append(c);
            break;
        }
    }

    if (!buffer.line.isEmpty() && (goAhead || buffer.type == TelnetDataEnum::UNKNOWN)) {
        {
            if (goAhead) {
                buffer.type = TelnetDataEnum::PROMPT;
                que.enqueue(buffer);
                buffer.line.clear();
                buffer.type = TelnetDataEnum::UNKNOWN;
            } else if (buffer.line.endsWith(ASCII_LF)) {
                buffer.type = TelnetDataEnum::LF;
                que.enqueue(buffer);
                buffer.line.clear();
                buffer.type = TelnetDataEnum::UNKNOWN;
            }
        }
    }
}
