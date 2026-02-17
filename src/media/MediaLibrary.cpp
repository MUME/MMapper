// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "MediaLibrary.h"

#include "../configuration/configuration.h"
#include "../global/ConfigConsts-Computed.h"

#ifndef MMAPPER_NO_AUDIO
#include <QMediaFormat>
#include <QMimeType>
#endif

#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QImageReader>
#include <QSet>
#include <QtGlobal>

MediaLibrary::MediaLibrary(QObject *const parent)
    : QObject(parent)
{
    // Initialize audio extensions (some backends do not list supported formats)
    QSet<QString> extensions;
#ifndef MMAPPER_NO_AUDIO
    extensions = {"aac", "m4a", "mp3", "mp4", "wav", "wave"};
    if (CURRENT_PLATFORM == PlatformEnum::Wasm) {
        extensions << "oga" << "ogg" << "opus" << "webm";
    }
    QMediaFormat format;
    for (auto f : format.supportedFileFormats(QMediaFormat::Decode)) {
        QMediaFormat info(f);
        QMimeType mime = info.mimeType();
        if (mime.isValid() && mime.name().startsWith(u"audio/")) {
            for (const QString &suffix : mime.suffixes()) {
                extensions << suffix.toLower();
            }
        }
    }
#endif
    m_audioExtensions = extensions.values();
    m_audioExtensions.sort(Qt::CaseInsensitive);
    qInfo() << "Supported Audio Formats:" << m_audioExtensions;

    // Initialize image extensions
    const QByteArrayList supportedFormatsList = QImageReader::supportedImageFormats();
    for (const QByteArray &formatName : supportedFormatsList) {
        m_imageExtensions.append(QString::fromUtf8(formatName).toLower());
    }
    m_imageExtensions.sort(Qt::CaseInsensitive);
    qInfo() << "Supported Image Formats:" << m_imageExtensions;

    scanDirectories();

    const auto &resourcesDir = getConfig().canvas.resourcesDirectory;
    auto addPathIfExist = [this](const QString &path) {
        if (QDir(path).exists()) {
            m_watcher.addPath(path);
        }
    };
    addPathIfExist(resourcesDir + "/areas");
    addPathIfExist(resourcesDir + "/rooms");
    addPathIfExist(resourcesDir + "/sounds");

    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString &) {
        scanDirectories();
        emit sig_mediaChanged();
    });
}

QString MediaLibrary::findAudio(const QString &subDir, const QString &name) const
{
    QString key = subDir + "/" + name;
    auto it = m_audioFiles.find(key);
    if (it != m_audioFiles.end()) {
        return it.value();
    }
    return "";
}

QString MediaLibrary::findImage(const QString &subDir, const QString &name) const
{
    QString key = subDir + "/" + name;
    auto it = m_imageFiles.find(key);
    if (it != m_imageFiles.end()) {
        return it.value();
    }
    return "";
}

void MediaLibrary::scanDirectories()
{
    m_audioFiles.clear();
    m_imageFiles.clear();

    auto scanPath = [&](const QString &path) {
        QDirIterator it(path, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QFileInfo fileInfo(it.next());
            QString suffix = fileInfo.suffix().toLower();
            QString filePath = fileInfo.filePath();

            auto dotIndex = filePath.lastIndexOf('.');
            if (dotIndex == -1) {
                continue;
            }

            const bool isQrc = filePath.startsWith(QLatin1String(":/"));
            QString baseName;
            if (isQrc) {
                baseName = filePath.left(dotIndex).mid(2); // remove :/
            } else {
                QString resourcesPath = getConfig().canvas.resourcesDirectory;
                if (!filePath.startsWith(resourcesPath)) {
                    continue;
                }
                baseName = filePath.mid(resourcesPath.length())
                               .left(dotIndex - resourcesPath.length());
                if (baseName.startsWith(QLatin1String("/"))) {
                    baseName = baseName.mid(1);
                }
            }

            if (baseName.isEmpty()) {
                continue;
            }

            if (m_audioExtensions.contains(suffix)) {
                m_audioFiles.insert(baseName, filePath);
            }
            if (m_imageExtensions.contains(suffix)) {
                m_imageFiles.insert(baseName, filePath);
            }
        }
    };

    // Scan QRC first then disk to prioritize disk
    scanPath(QLatin1String(":/areas"));
    scanPath(QLatin1String(":/rooms"));
    scanPath(QLatin1String(":/sounds"));

    const auto &resourcesDir = getConfig().canvas.resourcesDirectory;
    scanPath(resourcesDir + "/areas");
    scanPath(resourcesDir + "/rooms");
    scanPath(resourcesDir + "/sounds");

    qInfo() << "Scanned media directories. Found" << m_audioFiles.size() << "audio files and"
            << m_imageFiles.size() << "image files.";
}
