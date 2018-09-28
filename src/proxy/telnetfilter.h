#pragma once
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

#ifndef TELNETFILTER
#define TELNETFILTER

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
                              TelnetIncomingDataQueue &que);

    IncomingData m_userIncomingData{};
    IncomingData m_mudIncomingBuffer{};
    TelnetIncomingDataQueue m_mudIncomingQue{};
    TelnetIncomingDataQueue m_userIncomingQue{};
};

#endif
