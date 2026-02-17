// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "SfxManager.h"

#include "../configuration/configuration.h"
#include "MediaLibrary.h"

#ifndef MMAPPER_NO_AUDIO
#include <QAudioOutput>
#include <QMediaPlayer>
#endif

#include <QUrl>

SfxManager::SfxManager(const MediaLibrary &library, QObject *const parent)
    : QObject(parent)
    , m_library(library)
{
#ifndef MMAPPER_NO_AUDIO
    m_output = new QAudioOutput(this);
#endif
    updateVolume();
}

void SfxManager::playSound(MAYBE_UNUSED const QString &soundName)
{
    auto &cfg = getConfig().audio;
    if (!cfg.isUnlocked() || cfg.getSoundVolume() <= 0) {
        return;
    }

    QString path = m_library.findAudio("sounds", soundName);
    if (path.isEmpty()) {
        return;
    }

#ifndef MMAPPER_NO_AUDIO
    auto *effect = new QMediaPlayer(this);
    effect->setAudioOutput(m_output);
    if (path.startsWith(":/")) {
        effect->setSource(QUrl(QStringLiteral("qrc") + path));
    } else {
        effect->setSource(QUrl::fromLocalFile(path));
    }
    connect(effect,
            &QMediaPlayer::playbackStateChanged,
            this,
            [effect](QMediaPlayer::PlaybackState state) {
                if (state != QMediaPlayer::PlaybackState::PlayingState) {
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
