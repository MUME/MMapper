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

enum class TelnetDataEnum { UNKNOWN, PROMPT, MENU_PROMPT, LOGIN, LOGIN_PASSWORD, CRLF, LF, DELAY };

struct TelnetData final
{
    TelnetData() = default;
    QByteArray line;
    TelnetDataEnum type = TelnetDataEnum::UNKNOWN;
};

class TelnetFilter final : public QObject
{
    Q_OBJECT
    using TelnetIncomingDataQueue = QQueue<TelnetData>;

public:
    explicit TelnetFilter(QObject *const parent)
        : QObject(parent)
    {}
    ~TelnetFilter() override = default;

public slots:
    void onAnalyzeMudStream(const QByteArray &ba, bool goAhead);
    void onAnalyzeUserStream(const QByteArray &ba, bool goAhead);

signals:
    void parseNewMudInput(const TelnetData &);
    void parseNewUserInput(const TelnetData &);

private:
    void dispatchTelnetStream(const QByteArray &stream,
                              TelnetData &m_incomingData,
                              TelnetIncomingDataQueue &que,
                              const bool &goAhead);

    TelnetData m_userIncomingData;
    TelnetData m_mudIncomingBuffer;
    TelnetIncomingDataQueue m_mudIncomingQue;
    TelnetIncomingDataQueue m_userIncomingQue;
};
