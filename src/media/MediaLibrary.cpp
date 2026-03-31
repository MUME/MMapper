// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "MediaLibrary.h"

#include "../configuration/configuration.h"
#include "../display/Filenames.h"
#include "../global/ConfigConsts-Computed.h"

#ifndef MMAPPER_NO_AUDIO
#include <QMediaFormat>
#include <QMimeType>
#endif

#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QImageReader>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSet>
#include <QUrl>
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

    if constexpr (CURRENT_PLATFORM == PlatformEnum::Wasm) {
        m_network = new QNetworkAccessManager(this);
    }

    scanDirectories();

    if constexpr (CURRENT_PLATFORM != PlatformEnum::Wasm) {
        // Watch the assets directory too
        const QString assetsDir = getAssetsPath();
        if (QDir(assetsDir).exists()) {
            m_watcher.addPath(assetsDir);
        }

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

void MediaLibrary::loadManifest()
{
    if constexpr (CURRENT_PLATFORM != PlatformEnum::Wasm) {
        return;
    }

    const QString dir = getAssetsPath();

    // Fetch the manifest JSON from the network (it's served alongside the WASM files).
    auto *reply = m_network->get(QNetworkRequest(QUrl(dir + "manifest.json")));
    connect(reply, &QNetworkReply::finished, this, [this, dir, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "Failed to fetch asset manifest:" << reply->errorString();
            reply->deleteLater();
            return;
        }
        processManifest(reply->readAll(), dir);
        reply->deleteLater();
    });
}

void MediaLibrary::processManifest(const QByteArray &data, const QString &dir)
{
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        qWarning() << "Asset manifest is not a JSON array";
        return;
    }
    const QJsonArray arr = doc.array();
    for (const QJsonValue val : arr) {
        const QString relativePath = val.toString(); // e.g., "areas/weathertop.jpg"
        QFileInfo relInfo(relativePath);
        QString pathPart = relInfo.path();
        QString namePart = relInfo.completeBaseName();
        QString base;
        if (pathPart == "." || pathPart.isEmpty()) {
            base = namePart;
        } else {
            base = pathPart + "/" + namePart;
        }
        QString suffix = relInfo.suffix().toLower();

        // Construct the full path (dir is already set correctly by loadManifest())
        const QString fullPath = dir + relativePath;

        if (m_audioExtensions.contains(suffix)) {
            m_audioFiles.insert(base, fullPath);
        }
        if (m_imageExtensions.contains(suffix)) {
            m_imageFiles.insert(base, fullPath);
        }
    }
    qInfo() << "Loaded manifest with" << arr.size() << "assets.";
    emit sig_mediaChanged();
}

void MediaLibrary::fetchAsync(const QString &path, std::function<void(const QByteArray &)> callback)
{
    if constexpr (CURRENT_PLATFORM == PlatformEnum::Wasm) {
        if (path.isEmpty() || m_pendingFetches.contains(path)) {
            return;
        }

        m_pendingFetches.insert(path);

        auto *reply = m_network->get(QNetworkRequest(QUrl(path)));

        connect(reply, &QNetworkReply::finished, this, [this, reply, path, callback]() {
            m_pendingFetches.remove(path);

            QByteArray data;
            if (reply->error() == QNetworkReply::NoError) {
                data = reply->readAll();
            } else {
                qWarning() << "Network error fetching" << path << ":" << reply->errorString();
            }

            if (callback) {
                callback(data);
            }

            reply->deleteLater();
        });
    }
}

void MediaLibrary::scanPath(const QString &path, const QString &rootPath)
{
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
            if (!filePath.startsWith(rootPath)) {
                continue;
            }
            baseName = filePath.mid(rootPath.length()).left(dotIndex - rootPath.length());
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
}

void MediaLibrary::scanDirectories()
{
    m_audioFiles.clear();
    m_imageFiles.clear();

    // Scan resources in increasing order of priority: QRC < Sideloaded Assets < User Resources.
    // Duplicate keys encountered later will overwrite earlier ones.
    scanPath(QLatin1String(":/areas"), QLatin1String(":/"));
    scanPath(QLatin1String(":/rooms"), QLatin1String(":/"));
    scanPath(QLatin1String(":/sounds"), QLatin1String(":/"));

    if constexpr (CURRENT_PLATFORM != PlatformEnum::Wasm) {
        const QString &assetsDir = getAssetsPath();
        scanPath(assetsDir + "areas", assetsDir);
        scanPath(assetsDir + "rooms", assetsDir);
        scanPath(assetsDir + "sounds", assetsDir);
    }

    const auto &resourcesDir = getConfig().canvas.resourcesDirectory;
    scanPath(resourcesDir + "/areas", resourcesDir);
    scanPath(resourcesDir + "/rooms", resourcesDir);
    scanPath(resourcesDir + "/sounds", resourcesDir);

    qInfo() << "Scanned media directories. Found" << m_audioFiles.size() << "audio files and"
            << m_imageFiles.size() << "image files.";

    loadManifest();
}
