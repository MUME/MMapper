#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../global/macros.h"

#include <QFileSystemWatcher>
#include <QMap>
#include <QObject>
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

public:
    explicit MediaLibrary(QObject *parent = nullptr);

    QString findAudio(const QString &subDir, const QString &name) const;
    QString findImage(const QString &subDir, const QString &name) const;

signals:
    void sig_mediaChanged();

private:
    void scanDirectories();
};
