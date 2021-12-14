#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QString>

#include "../global/Array.h"

class NODISCARD CompareVersion final
{
private:
    MMapper::Array<int, 3> parts;

public:
    explicit CompareVersion(const QString &versionStr) noexcept;

public:
    bool operator>(const CompareVersion &other) const;
    bool operator==(const CompareVersion &other) const;

    int major() const { return parts[0]; }
    int minor() const { return parts[1]; }
    int patch() const { return parts[2]; }

public:
    NODISCARD QString toQString() const;
    explicit operator QString() const { return toQString(); }
    friend QDebug operator<<(QDebug os, const CompareVersion &compareVersion)
    {
        return os << compareVersion.toQString();
    }
};

class UpdateDialog : public QDialog
{
    Q_OBJECT
private:
    QNetworkAccessManager manager;
    QString downloadUrl;
    QLabel *text = nullptr;
    QDialogButtonBox *buttonBox = nullptr;

public:
    explicit UpdateDialog(QWidget *parent);

    void open() override;

private slots:
    void managerFinished(QNetworkReply *reply);
};
