#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Nils Schimmelmann <nschimme@gmail.com> (Jahara)

#include "../global/Array.h"
#include "../global/macros.h"

#include <QDebug>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>

class QJsonObject;
class QNetworkReply;

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

// Widget-free network/version-check logic, so the same implementation can
// back any UI showing update status. Owns the single QNetworkAccessManager
// instance for the check; see check()/upgrade().
class NODISCARD_QOBJECT UpdateChecker final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString statusText READ getStatusText NOTIFY sig_statusTextChanged)
    Q_PROPERTY(bool upgradeAvailable READ isUpgradeAvailable NOTIFY sig_upgradeAvailableChanged)

private:
    QNetworkAccessManager m_manager;
    QString m_downloadUrl;
    QString m_statusText;
    bool m_upgradeAvailable = false;

public:
    explicit UpdateChecker(QObject *parent);

    NODISCARD const QString &getStatusText() const { return m_statusText; }
    NODISCARD bool isUpgradeAvailable() const { return m_upgradeAvailable; }

    // Starts (or restarts) a check against GitHub for a newer release.
    Q_INVOKABLE void check();

    // Opens the discovered download URL in the user's browser. Returns
    // false (and does nothing else) if there is no URL yet, or the OS
    // failed to open it.
    Q_INVOKABLE bool upgrade();

signals:
    void sig_statusTextChanged();
    void sig_upgradeAvailableChanged();
    // Emitted when a newer version was found, so a UI showing this checker
    // should raise itself in front of the user.
    void sig_showDialog();

private slots:
    void managerFinished(QNetworkReply *reply);

private:
    void setUpdateStatus(const QString &message, bool enableUpgradeButton, bool showDialog);
    NODISCARD QString findDownloadUrlForRelease(const QJsonObject &releaseObject) const;
};
