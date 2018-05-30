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

#ifndef REMOTEEDIT_H
#define REMOTEEDIT_H

#include <QObject>
#include <QRegExp>

class RemoteEditWidget;

class RemoteEdit : public QObject
{
    Q_OBJECT

public:
    RemoteEdit(QObject *parent = 0);
    ~RemoteEdit();

    static const QRegExp s_lineFeedNewlineRx;

public slots:
    void remoteView(const QString &, const QString &);
    void remoteEdit(const int, const QString &, const QString &);

protected slots:
    void cancel(const int);
    void save(const QString &, const int);

signals:
    void sendToSocket(const QByteArray &);
};

#endif /* REMOTEEDIT_H */
