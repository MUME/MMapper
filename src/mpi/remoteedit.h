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

#include "remoteeditsession.h"

#include <memory>
#include <QObject>
#include <QRegExp>

class RemoteEdit : public QObject
{
    Q_OBJECT

    friend class RemoteEditSession;

public:
    RemoteEdit(QObject *parent = nullptr)
        : QObject(parent)
    {}
    ~RemoteEdit() = default;

public slots:
    void remoteView(const QString &, const QString &);
    void remoteEdit(const int, const QString &, const QString &);

signals:
    void sendToSocket(const QByteArray &);

protected:
    void cancel(const RemoteEditSession *);
    void save(const RemoteEditSession *);

private:
    uint getSessionCount() { return greatestUsedId == UINT_MAX ? 0 : greatestUsedId + 1; }
    void addSession(const int, const QString &, QString);
    void removeSession(const uint);

    static const QRegExp s_lineFeedNewlineRx;

    uint greatestUsedId{0};
    std::map<int, std::unique_ptr<RemoteEditSession>> m_sessions;
};

#endif /* REMOTEEDIT_H */
