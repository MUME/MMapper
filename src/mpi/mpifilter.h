#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QtCore>

#include "../proxy/telnetfilter.h"

class MpiFilter final : public QObject
{
    Q_OBJECT
private:
    TelnetDataEnum m_previousType = TelnetDataEnum::UNKNOWN;
    bool m_receivingMpi = false;

    char m_command = '\0';
    int m_remaining = 0;
    QByteArray m_buffer;

public:
    explicit MpiFilter(QObject *const parent)
        : QObject(parent)
    {}

signals:
    void sendToMud(const QByteArray &);
    void parseNewMudInput(const IncomingData &data);
    void editMessage(int, const QString &, const QString &);
    void viewMessage(const QString &, const QString &);

public slots:
    void analyzeNewMudInput(const IncomingData &data);

protected:
    void parseMessage(char command, const QByteArray &buffer);
    void parseEditMessage(const QByteArray &buffer);
    void parseViewMessage(const QByteArray &buffer);
};
