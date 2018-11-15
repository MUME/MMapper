#pragma once
/************************************************************************
**
** Authors:   Azazello <lachupe@gmail.com>,
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

#ifndef CGROUPSERVER_H_
#define CGROUPSERVER_H_

#include <QByteArray>
#include <QList>
#include <QString>
#include <QTcpServer>
#include <QtCore>
#include <QtGlobal>

class CGroupServerCommunicator;
class CGroupClient;
class QObject;

class CGroupServer final : public QTcpServer
{
    Q_OBJECT

public:
    explicit CGroupServer(CGroupServerCommunicator *parent);
    virtual ~CGroupServer() override;

    void sendToAll(const QByteArray &);
    void sendToAllExceptOne(CGroupClient *exception, const QByteArray &);
    void closeAll();

protected slots:
    void errorInConnection(CGroupClient *, const QString &);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

signals:
    void sendLog(const QString &);
    void connectionClosed(CGroupClient *);

private:
    void connectAll(CGroupClient *);
    void disconnectAll(CGroupClient *);

    QList<CGroupClient *> connections{};
    CGroupServerCommunicator *communicator;
};

#endif /*CGROUPSERVER_H_*/
