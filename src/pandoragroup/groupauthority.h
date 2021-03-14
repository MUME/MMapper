#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QObject>
#include <QSslCertificate>
#include <QSslKey>
#include <QStringListModel>

#include "../global/macros.h"

enum class NODISCARD GroupMetadataEnum { LAST_LOGIN, NAME, IP_ADDRESS, CERTIFICATE, PORT };
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

private:
    void refresh();

public slots:
    void slot_refresh() { refresh(); }

public:
    NODISCARD GroupSecret getSecret() const;
    NODISCARD QSslCertificate getLocalCertificate() const { return certificate; }
    NODISCARD QSslKey getPrivateKey() const { return key; }

public:
    NODISCARD QAbstractItemModel *getItemModel() { return &model; }
    bool add(const GroupSecret &);
    bool remove(const GroupSecret &);
    NODISCARD bool validSecret(const GroupSecret &) const;
    NODISCARD bool validCertificate(const GroupSocket *) const;

public:
    NODISCARD QString getMetadata(const GroupSecret &, GroupMetadataEnum) const;
    static void setMetadata(const GroupSecret &, GroupMetadataEnum, const QString &value);

signals:
    void sig_secretRevoked(const GroupSecret &);
    void sig_secretRefreshed(const GroupSecret &);

private:
    QStringListModel model;
    QSslCertificate certificate;
    QSslKey key;
};
