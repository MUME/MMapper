#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/macros.h"

#include <cstdint>

#include <QByteArray>
#include <QObject>
#include <QQueue>
#include <QString>
#include <QtCore>

enum class NODISCARD TelnetDataEnum : uint8_t { UNKNOWN, PROMPT, CRLF, LF, DELAY };

struct NODISCARD TelnetData final
{
    TelnetData() = default;
    QByteArray line;
    TelnetDataEnum type = TelnetDataEnum::UNKNOWN;
};

class TelnetFilter final : public QObject
{
    Q_OBJECT

public:
    using TelnetIncomingDataQueue = QQueue<TelnetData>;

private:
    TelnetData m_userIncomingData;
    TelnetData m_mudIncomingBuffer;
    TelnetIncomingDataQueue m_mudIncomingQue;
    TelnetIncomingDataQueue m_userIncomingQue;

public:
    explicit TelnetFilter(QObject *const parent)
        : QObject(parent)
    {}
    ~TelnetFilter() final = default;

public slots:
    void slot_onAnalyzeMudStream(const QByteArray &ba, bool goAhead);
    void slot_onAnalyzeUserStream(const QByteArray &ba, bool goAhead);

signals:
    void sig_parseNewMudInput(const TelnetData &);
    void sig_parseNewUserInput(const TelnetData &);

private:
    void dispatchTelnetStream(const QByteArray &stream,
                              TelnetData &m_incomingData,
                              TelnetIncomingDataQueue &que,
                              const bool &goAhead);
};
