#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve)
// Author: Marek Krejza <krejza@gmail.com> (Caligor)
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/macros.h"
#include "TaggedBytes.h"

#include <cstdint>

#include <QByteArray>
#include <QObject>
#include <QQueue>
#include <QString>
#include <QtCore>

enum class NODISCARD TelnetDataEnum : uint8_t { Unknown, Prompt, CRLF, LF, Delay };

struct NODISCARD TelnetData final
{
    TelnetData() = default;
    RawBytes line;
    TelnetDataEnum type = TelnetDataEnum::Unknown;
};

using TelnetIncomingDataQueue = QQueue<TelnetData>;

class NODISCARD_QOBJECT MudTelnetFilter final : public QObject
{
    Q_OBJECT

private:
    TelnetData m_mudIncomingBuffer;

public:
    explicit MudTelnetFilter(QObject *const parent)
        : QObject(parent)
    {}
    ~MudTelnetFilter() final = default;

signals:
    void sig_parseNewMudInput(const TelnetData &);

public slots:
    void slot_onAnalyzeMudStream(const RawBytes &ba, bool goAhead);
};

class NODISCARD_QOBJECT UserTelnetFilter final : public QObject
{
    Q_OBJECT

private:
    TelnetData m_userIncomingData;

public:
    explicit UserTelnetFilter(QObject *const parent)
        : QObject(parent)
    {}
    ~UserTelnetFilter() final = default;

signals:
    void sig_parseNewUserInput(const TelnetData &);

public slots:
    void slot_onAnalyzeUserStream(const RawBytes &ba, bool goAhead);
};
