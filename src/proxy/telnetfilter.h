#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QByteArray>
#include <QObject>
#include <QQueue>
#include <QString>
#include <QtCore>

enum class TelnetDataType {
    PROMPT,
    MENU_PROMPT,
    LOGIN,
    LOGIN_PASSWORD,
    CRLF,
    LFCR,
    LF,
    DELAY,
    SPLIT,
    UNKNOWN
};

struct IncomingData
{
    explicit IncomingData() = default;
    QByteArray line{};
    TelnetDataType type = TelnetDataType::SPLIT;
};

class TelnetFilter : public QObject
{
    Q_OBJECT
    using TelnetIncomingDataQueue = QQueue<IncomingData>;

public:
    explicit TelnetFilter(QObject *parent);
    ~TelnetFilter() = default;

public slots:
    void onAnalyzeMudStream(const QByteArray &ba, bool goAhead);
    void onAnalyzeUserStream(const QByteArray &ba, bool goAhead);

signals:
    void parseNewMudInput(const IncomingData &);
    void parseNewUserInput(const IncomingData &);

private:
    void dispatchTelnetStream(const QByteArray &stream,
                              IncomingData &m_incomingData,
                              TelnetIncomingDataQueue &que,
                              const bool &goAhead);

    IncomingData m_userIncomingData{};
    IncomingData m_mudIncomingBuffer{};
    TelnetIncomingDataQueue m_mudIncomingQue{};
    TelnetIncomingDataQueue m_userIncomingQue{};
};
