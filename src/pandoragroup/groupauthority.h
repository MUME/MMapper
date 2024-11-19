#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/macros.h"
#include "../proxy/TaggedBytes.h"

#include <QObject>
#include <QSslCertificate>
#include <QSslKey>
#include <QStringListModel>

enum class NODISCARD GroupMetadataEnum { LAST_LOGIN, NAME, IP_ADDRESS, CERTIFICATE, PORT };
static constexpr const auto NUM_GROUP_METADATA = 4u;

static constexpr const auto GROUP_ORGANIZATION = "MUME";
static constexpr const auto GROUP_ORGANIZATIONAL_UNIT = "MMapper";
static constexpr const auto GROUP_COMMON_NAME = "GroupManager";

class GroupSocket;
class NODISCARD_QOBJECT GroupAuthority : public QObject
{
    Q_OBJECT

private:
    QStringListModel model;
    QSslCertificate certificate;
    QSslKey key;

public:
    explicit GroupAuthority(QObject *parent);

private:
    void refresh();

public:
    NODISCARD GroupSecret getSecret() const;
    NODISCARD QSslCertificate getLocalCertificate() const { return certificate; }
    NODISCARD QSslKey getPrivateKey() const { return key; }

public:
    NODISCARD QAbstractItemModel *getItemModel() { return &model; }
    void add(const GroupSecret &);
    void remove(const GroupSecret &);
    NODISCARD bool validSecret(const GroupSecret &) const;
    NODISCARD static bool validCertificate(const GroupSocket &);

public:
    NODISCARD static QString getMetadata(const GroupSecret &, GroupMetadataEnum);
    static void setMetadata(const GroupSecret &, GroupMetadataEnum, const QString &value);

signals:
    void sig_secretRevoked(const GroupSecret &);
    void sig_secretRefreshed(const GroupSecret &);

public slots:
    void slot_refresh() { refresh(); }
};
