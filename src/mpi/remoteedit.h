#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <climits>
#include <map>
#include <memory>
#include <QByteArray>
#include <QObject>
#include <QString>
#include <QtCore>
#include <QtGlobal>

#include "remoteeditsession.h"

class RemoteEditSession;

class RemoteEdit : public QObject
{
    Q_OBJECT
    friend class RemoteEditSession;

public:
    explicit RemoteEdit(QObject *parent = nullptr)
        : QObject(parent)
    {}
    ~RemoteEdit() = default;

public slots:
    void remoteView(const QString &, const QString &);
    void remoteEdit(int, const QString &, const QString &);

signals:
    void sendToSocket(const QByteArray &);

protected:
    void cancel(const RemoteEditSession *);
    void save(const RemoteEditSession *);

private:
    uint getSessionCount() { return greatestUsedId == UINT_MAX ? 0 : greatestUsedId + 1; }
    void addSession(int, const QString &, QString);
    void removeSession(uint);

    uint greatestUsedId = 0;
    std::map<uint, std::unique_ptr<RemoteEditSession>> m_sessions;
};
