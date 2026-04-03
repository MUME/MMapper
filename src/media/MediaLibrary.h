#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../global/macros.h"

#include <functional>

#include <QFileSystemWatcher>
#include <QImage>
#include <QMap>
#include <QNetworkAccessManager>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>

class NODISCARD_QOBJECT MediaLibrary final : public QObject
{
    Q_OBJECT

private:
    QFileSystemWatcher m_watcher;
    QMap<QString, QString> m_audioFiles;
    QMap<QString, QString> m_imageFiles;

    QStringList m_audioExtensions;
    QStringList m_imageExtensions;

    QNetworkAccessManager *m_network = nullptr;
    QSet<QString> m_pendingFetches;

public:
    explicit MediaLibrary(QObject *parent = nullptr);

    NODISCARD QString findAudio(const QString &subDir, const QString &name) const;
    NODISCARD QString findImage(const QString &subDir, const QString &name) const;

    void fetchAsync(const QString &path, std::function<void(const QByteArray &)> callback);

signals:
    void sig_mediaChanged();

private:
    void scanDirectories();
    void scanPath(const QString &path, const QString &rootPath);
    void loadManifest();
    void processManifest(const QByteArray &data, const QString &dir);
};
