#pragma once
/************************************************************************
**
** Authors:   Nils Schimmelmann <nschimme@gmail.com> (Jahara)
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

#include "AbstractTelnet.h"
#include <QByteArray>
#include <QObject>

class UserTelnet final : public AbstractTelnet
{
    Q_OBJECT
public:
    explicit UserTelnet(QObject *parent);
    ~UserTelnet() override = default;

public slots:
    void onSendToUser(const QByteArray &data, bool goAhead);
    void onAnalyzeUserStream(const QByteArray &);
    void onConnected();
    void onRelayEchoMode(bool);

signals:
    void analyzeUserStream(const QByteArray &, bool goAhead);
    void sendToSocket(const QByteArray &);

    void relayNaws(int, int);
    void relayTermType(QByteArray);

private:
    void sendToMapper(const QByteArray &data, bool goAhead) override;
    void receiveTerminalType(const QByteArray &) override;
    void receiveWindowSize(int, int) override;
    void sendRawData(const QByteArray &data) override;
};
