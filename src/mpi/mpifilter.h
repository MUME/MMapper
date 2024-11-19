#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/Consts.h"
#include "../mpi/remoteeditsession.h"
#include "../proxy/telnetfilter.h"

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QtCore>

class NODISCARD_QOBJECT MpiFilter final : public QObject
{
    Q_OBJECT

private:
    TelnetDataEnum m_previousType = TelnetDataEnum::Unknown;
    bool m_receivingMpi = false;

    char m_command = char_consts::C_NUL;
    int m_remaining = 0;
    RawBytes m_buffer;

public:
    explicit MpiFilter(QObject *const parent)
        : QObject(parent)
    {}

protected:
    void parseMessage(char command, const RawBytes &buffer);
    void parseEditMessage(const RawBytes &buffer);
    void parseViewMessage(const RawBytes &buffer);

private:
    void parseNewMudInput(const TelnetData &data);

signals:
    void sig_parseNewMudInput(const TelnetData &data);
    void sig_editMessage(const RemoteSession &, const QString &, const QString &);
    void sig_viewMessage(const QString &, const QString &);

public slots:
    void slot_analyzeNewMudInput(const TelnetData &data);
};
