#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QObject>
#include <QSslCertificate>
#include <QSslKey>
#include <QStringListModel>

enum class GroupMetadata { LAST_LOGIN, NAME, IP_ADDRESS, CERTIFICATE, PORT };
static constexpr const auto NUM_GROUP_METADATA = 4u;

using GroupSecret = QByteArray;

static constexpr const auto GROUP_ORGANIZATION = "MUME";
static constexpr const auto GROUP_ORGANIZATIONAL_UNIT = "MMapper";
static constexpr const auto GROUP_COMMON_NAME = "GroupManager";

class GroupSocket;
class GroupAuthority : public QObject
{
    Q_OBJECT
public:
    explicit GroupAuthority(QObject *parent = nullptr);

public slots:
    void refresh();

public:
    GroupSecret getSecret() const;
    QSslCertificate getLocalCertificate() const { return certificate; }
    QSslKey getPrivateKey() const { return key; }

public:
    QAbstractItemModel *getItemModel() { return &model; }
    bool add(const GroupSecret &);
    bool remove(const GroupSecret &);
    bool validSecret(const GroupSecret &) const;
    bool validCertificate(const GroupSocket *) const;

public:
    QString getMetadata(const GroupSecret &, const GroupMetadata) const;
    void setMetadata(const GroupSecret &, const GroupMetadata, const QString &value);

signals:
    void secretRevoked(const GroupSecret &);
    void secretRefreshed(const GroupSecret &);

private:
    QStringListModel model;
    QSslCertificate certificate;
    QSslKey key;
};
