// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "telnetfilter.h"

#include "../global/Consts.h"

#include <QByteArray>
#include <QObject>

namespace { // anonymous

constexpr const char ASCII_DEL = char_consts::C_BACKSPACE;
constexpr const char ASCII_CR = char_consts::C_CARRIAGE_RETURN;
constexpr const char ASCII_LF = char_consts::C_NEWLINE;

static_assert(ASCII_DEL == 8);
static_assert(ASCII_LF == 10);
static_assert(ASCII_CR == 13);

static void dispatchTelnetStream(const QByteArray &stream,
                                 TelnetData &buffer,
                                 TelnetIncomingDataQueue &queue,
                                 const bool goAhead)
{
    for (const char c : stream) {
        switch (c) {
        case ASCII_DEL:
            buffer.line.append(ASCII_DEL);
            buffer.type = TelnetDataEnum::Delay;
            queue.enqueue(buffer);
            buffer.line.clear();
            buffer.type = TelnetDataEnum::Unknown;
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
                    queue.enqueue(buffer);
                    buffer.line.clear();
                    buffer.type = TelnetDataEnum::Unknown;
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
                queue.enqueue(buffer);
                buffer.line.clear();
                buffer.type = TelnetDataEnum::Unknown;
            }

            buffer.line.append(c);
            break;
        }
    }

    if (!buffer.line.isEmpty() && (goAhead || buffer.type == TelnetDataEnum::Unknown)) {
        {
            if (goAhead) {
                buffer.type = TelnetDataEnum::Prompt;
                queue.enqueue(buffer);
                buffer.line.clear();
                buffer.type = TelnetDataEnum::Unknown;
            } else if (buffer.line.endsWith(ASCII_LF)) {
                buffer.type = TelnetDataEnum::LF;
                queue.enqueue(buffer);
                buffer.line.clear();
                buffer.type = TelnetDataEnum::Unknown;
            }
        }
    }
}
} // namespace

void MudTelnetFilter::slot_onAnalyzeMudStream(const QByteArray &ba, const bool goAhead)
{
    TelnetIncomingDataQueue queue;
    dispatchTelnetStream(ba, m_mudIncomingBuffer, queue, goAhead);
    for (const auto &data : queue) {
        emit sig_parseNewMudInput(data);
    }
}

void UserTelnetFilter::slot_onAnalyzeUserStream(const QByteArray &ba, const bool goAhead)
{
    TelnetIncomingDataQueue queue;
    dispatchTelnetStream(ba, m_userIncomingData, queue, goAhead);
    for (const auto &data : queue) {
        emit sig_parseNewUserInput(data);
    }
}
