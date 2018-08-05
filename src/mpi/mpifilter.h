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

#ifndef MPIFILTER_H
#define MPIFILTER_H

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QtCore>

#include "../proxy/telnetfilter.h"

class MpiFilter : public QObject
{
    Q_OBJECT
public:
    explicit MpiFilter(QObject *parent = nullptr);

signals:
    void sendToMud(const QByteArray &);
    void parseNewMudInput(IncomingData &data);
    void editMessage(int, const QString &, const QString &);
    void viewMessage(const QString &, const QString &);

public slots:
    void analyzeNewMudInput(IncomingData &data);

protected:
    void parseMessage(char command, const QByteArray &buffer);
    void parseEditMessage(const QByteArray &buffer);
    void parseViewMessage(const QByteArray &buffer);

private:
    TelnetDataType m_previousType = TelnetDataType::UNKNOWN;
    bool m_parsingMpi = false;

    char m_command{}; /* '\0' */
    int m_remaining = 0;
    QByteArray m_buffer{};
};

#endif // MPIFILTER_H
