// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "SfxManager.h"

#include "../configuration/configuration.h"
#include "../global/ConfigConsts-Computed.h"
#include "MediaLibrary.h"

#ifndef MMAPPER_NO_AUDIO
#include <QAudioDevice>
#include <QAudioOutput>
#include <QMediaPlayer>
#endif

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QUrl>

SfxManager::SfxManager(MediaLibrary &library, QObject *const parent)
    : QObject(parent)
    , m_library(library)
{
#ifndef MMAPPER_NO_AUDIO
    m_output = new QAudioOutput(this);
#endif
    updateVolume();
}

SfxManager::~SfxManager()
{
    shutdown();
}

void SfxManager::shutdown()
{
#ifndef MMAPPER_NO_AUDIO
    const auto players = findChildren<QMediaPlayer *>();
    for (auto *player : players) {
        player->stop();
        player->setSource(QUrl());
    }
#endif
}

void SfxManager::playSound(MAYBE_UNUSED const QString &soundName)
{
    auto &cfg = getConfig().audio;
    if (!cfg.isUnlocked() || cfg.getSoundVolume() <= 0) {
        return;
    }

    const QString path = m_library.findAudio("sounds", soundName);
    if (path.isEmpty()) {
        return;
    }

    if constexpr (CURRENT_PLATFORM == PlatformEnum::Wasm) {
        m_library.fetchAsync(path,
                             [this, path](const QByteArray &data) { playFromData(data, path); });
    } else {
        const QUrl url = path.startsWith(":/") ? QUrl(QStringLiteral("qrc") + path)
                                               : QUrl::fromLocalFile(path);
        startEffect(url);
    }
}

void SfxManager::playFromData(const QByteArray &data, const QString &soundName)
{
#ifndef MMAPPER_NO_AUDIO
    if (data.isEmpty()) {
        qWarning() << "SfxManager: empty data for" << soundName;
        return;
    }

    auto &cfg = getConfig().audio;
    if (!cfg.isUnlocked() || cfg.getSoundVolume() <= 0) {
        return;
    }

    // Write to a unique temp file per playback to avoid collisions when the same
    // sound plays concurrently; startEffect deletes the file once playback ends.
    const QString suffix = QFileInfo(soundName).suffix();
    QTemporaryFile tmp(QDir::tempPath() + "/mmapper_XXXXXX." + suffix);
    if (!tmp.open() || tmp.write(data) != data.size()) {
        qWarning() << "SfxManager: failed to write temp file for" << soundName;
        return;
    }
    tmp.setAutoRemove(false); // startEffect's cleanup lambda will delete it
    const QString tmpPath = tmp.fileName();
    tmp.close();
    startEffect(QUrl::fromLocalFile(tmpPath), tmpPath);
#else
    std::ignore = data;
    std::ignore = soundName;
#endif
}

void SfxManager::startEffect(MAYBE_UNUSED const QUrl &url, MAYBE_UNUSED const QString &tmpToDelete)
{
#ifndef MMAPPER_NO_AUDIO
    auto *effect = new QMediaPlayer(this);
    effect->setAudioOutput(m_output);
    effect->setSource(url);
    connect(effect,
            &QMediaPlayer::playbackStateChanged,
            this,
            [effect, tmpToDelete](QMediaPlayer::PlaybackState state) {
                if (state != QMediaPlayer::PlaybackState::PlayingState) {
                    if (!tmpToDelete.isEmpty()) {
                        QFile::remove(tmpToDelete);
                    }
                    effect->deleteLater();
                }
            });
    effect->play();
#endif
}

void SfxManager::updateVolume()
{
#ifndef MMAPPER_NO_AUDIO
    m_output->setVolume(static_cast<float>(getConfig().audio.getSoundVolume()) / 100.0f);
#endif
}

#ifndef MMAPPER_NO_AUDIO
void SfxManager::updateOutputDevice(const QAudioDevice &device)
{
    if (m_output->device() != device) {
        m_output->setDevice(device);
    }
}
#endif
