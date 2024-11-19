#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/Array.h"

#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QString>

class NODISCARD CompareVersion final
{
private:
    MMapper::Array<int, 3> m_parts;

public:
    explicit CompareVersion(const QString &versionStr) noexcept;

public:
    NODISCARD bool operator>(const CompareVersion &other) const;
    NODISCARD bool operator==(const CompareVersion &other) const;

    NODISCARD int major() const { return m_parts[0]; }
    NODISCARD int minor() const { return m_parts[1]; }
    NODISCARD int patch() const { return m_parts[2]; }

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
    QNetworkAccessManager m_manager;
    QString m_downloadUrl;
    QLabel *m_text = nullptr;
    QDialogButtonBox *m_buttonBox = nullptr;

public:
    explicit UpdateDialog(QWidget *parent);

    void open() override;

private slots:
    void managerFinished(QNetworkReply *reply);
};
