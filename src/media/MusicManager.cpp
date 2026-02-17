// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "MusicManager.h"

#include "../configuration/configuration.h"
#include "MediaLibrary.h"

#ifndef MMAPPER_NO_AUDIO
#include <QAudioOutput>
#include <QMediaPlayer>
#include <QUrl>
#endif

#include <QTimer>

MusicManager::MusicManager(const MediaLibrary &library, QObject *const parent)
    : QObject(parent)
    , m_library(library)
{
#ifndef MMAPPER_NO_AUDIO
    m_cachedPositions.setMaxCost(10);
    for (int i = 0; i < 2; ++i) {
        m_channels[i].player = new QMediaPlayer(this);
        m_channels[i].audioOutput = new QAudioOutput(this);
        m_channels[i].player->setAudioOutput(m_channels[i].audioOutput);
        m_channels[i].player->setLoops(QMediaPlayer::Infinite);

        connect(m_channels[i].player,
                &QMediaPlayer::mediaStatusChanged,
                this,
                [this, i](QMediaPlayer::MediaStatus status) {
                    if (status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::BufferedMedia
                        || status == QMediaPlayer::BufferingMedia) {
                        applyPendingPosition(i);
                    }
                });

        connect(m_channels[i].player, &QMediaPlayer::seekableChanged, this, [this, i](bool seekable) {
            if (seekable) {
                applyPendingPosition(i);
            }
        });
    }

    m_fadeStep = static_cast<float>(FADE_INTERVAL_MS) / CROSSFADE_DURATION_MS;
    m_fadeTimer = new QTimer(this);
    m_fadeTimer->setInterval(FADE_INTERVAL_MS);

    connect(&m_library, &MediaLibrary::sig_mediaChanged, this, &MusicManager::slot_onMediaChanged);

    connect(m_fadeTimer, &QTimer::timeout, this, [this]() {
        bool changed = false;
        for (int i = 0; i < 2; ++i) {
            float target = 0.0f;
            if (!m_fadingToSilence && i == m_activeChannel) {
                target = 1.0f;
            }

            if (m_channels[i].fadeVolume < target) {
                m_channels[i].fadeVolume = std::min(target, m_channels[i].fadeVolume + m_fadeStep);
                changed = true;
            } else if (m_channels[i].fadeVolume > target) {
                m_channels[i].fadeVolume = std::max(target, m_channels[i].fadeVolume - m_fadeStep);
                changed = true;
            }
            updateChannelVolume(i);
        }

        if (!changed) {
            m_fadeTimer->stop();
            int inactive = (m_activeChannel + 1) % 2;
            if (m_channels[inactive].fadeVolume <= 0.0f) {
                if (!m_channels[inactive].file.isEmpty()) {
                    if (m_channels[inactive].player->playbackState() == QMediaPlayer::PlayingState) {
                        m_cachedPositions.insert(m_channels[inactive].file,
                                                 new qint64(
                                                     m_channels[inactive].player->position()));
                    }
                    m_channels[inactive].player->stop();
                    m_channels[inactive].file.clear();
                }
            }
            if (m_fadingToSilence && m_channels[m_activeChannel].fadeVolume <= 0.0f) {
                if (m_channels[m_activeChannel].player->playbackState()
                    == QMediaPlayer::PlayingState) {
                    m_cachedPositions.insert(m_channels[m_activeChannel].file,
                                             new qint64(
                                                 m_channels[m_activeChannel].player->position()));
                }
                m_channels[m_activeChannel].player->stop();
                m_channels[m_activeChannel].file.clear();
            }
        }
    });
#endif
}

MusicManager::~MusicManager()
{
#ifndef MMAPPER_NO_AUDIO
    for (int i = 0; i < 2; ++i) {
        m_channels[i].player->stop();
    }
#endif
}

void MusicManager::playMusic(const QString &musicFile)
{
#ifndef MMAPPER_NO_AUDIO
    if (musicFile.isEmpty()) {
        stopMusic();
        return;
    }

    if (musicFile == m_channels[m_activeChannel].file) {
        startFade(false);
        return;
    }

    int inactiveChannel = (m_activeChannel + 1) % 2;
    if (musicFile == m_channels[inactiveChannel].file) {
        m_activeChannel = inactiveChannel;
        startFade(false);
        return;
    }

    // New file: start crossfade
    if (!m_channels[m_activeChannel].file.isEmpty()
        && m_channels[m_activeChannel].player->playbackState() == QMediaPlayer::PlayingState) {
        m_cachedPositions.insert(m_channels[m_activeChannel].file,
                                 new qint64(m_channels[m_activeChannel].player->position()));
    }

    m_activeChannel = inactiveChannel;
    auto &newActive = m_channels[m_activeChannel];
    newActive.file = musicFile;
    newActive.fadeVolume = 0.0f;

    if (qint64 *pos = m_cachedPositions.object(newActive.file)) {
        newActive.pendingPosition = *pos;
    } else {
        newActive.pendingPosition = -1;
    }

    if (newActive.file.startsWith(":")) {
        newActive.player->setSource(QUrl("qrc" + newActive.file));
    } else {
        newActive.player->setSource(QUrl::fromLocalFile(newActive.file));
    }

    auto &cfg = getConfig().audio;
    bool canPlay = cfg.isUnlocked() && cfg.getMusicVolume() > 0;
    if (canPlay) {
        newActive.player->play();
        applyPendingPosition(m_activeChannel);
    }

    startFade(false);
#else
    std::ignore = musicFile;
#endif
}

void MusicManager::stopMusic()
{
#ifndef MMAPPER_NO_AUDIO
    startFade(true);
#endif
}

void MusicManager::updateVolumes()
{
#ifndef MMAPPER_NO_AUDIO
    for (int i = 0; i < 2; ++i) {
        updateChannelVolume(i);
    }

    auto &cfg = getConfig().audio;
    float masterVol = static_cast<float>(cfg.getMusicVolume()) / 100.0f;
    bool canPlay = cfg.isUnlocked() && masterVol > 0;
    auto &active = m_channels[m_activeChannel];
    if (canPlay && active.player->playbackState() == QMediaPlayer::StoppedState
        && !active.file.isEmpty()) {
        if (qint64 *pos = m_cachedPositions.object(active.file)) {
            active.pendingPosition = *pos;
        }
        active.player->play();
        applyPendingPosition(m_activeChannel);
    } else if (masterVol <= 0 && active.player->playbackState() == QMediaPlayer::PlayingState) {
        if (!active.file.isEmpty()) {
            m_cachedPositions.insert(active.file, new qint64(active.player->position()));
        }
        active.player->stop();
    }
#endif
}

void MusicManager::slot_onMediaChanged()
{
#ifndef MMAPPER_NO_AUDIO
    m_cachedPositions.clear();
    QString currentFile = m_channels[m_activeChannel].file;
    if (!currentFile.isEmpty()) {
        m_channels[m_activeChannel].file.clear();
        playMusic(currentFile);
    }
#endif
}

#ifndef MMAPPER_NO_AUDIO
void MusicManager::applyPendingPosition(int channelIndex)
{
    auto &ch = m_channels[channelIndex];
    if (ch.pendingPosition != -1 && ch.player->isSeekable()) {
        ch.player->setPosition(ch.pendingPosition);
        ch.pendingPosition = -1;
    }
}

void MusicManager::updateChannelVolume(int channelIndex)
{
    float masterVol = static_cast<float>(getConfig().audio.getMusicVolume()) / 100.0f;
    m_channels[channelIndex].audioOutput->setVolume(masterVol * m_channels[channelIndex].fadeVolume);
}

void MusicManager::startFade(bool toSilence)
{
    m_fadingToSilence = toSilence;
    if (m_fadeTimer && !m_fadeTimer->isActive()) {
        m_fadeTimer->start();
    }
}
#endif
