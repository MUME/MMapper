// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "AudioManager.h"

#include "../configuration/configuration.h"
#include "../global/Charset.h"
#include "../map/mmapper2room.h"
#include "MediaLibrary.h"
#include "MusicManager.h"
#include "SfxManager.h"

#include <QRegularExpression>

AudioManager::AudioManager(MediaLibrary &library, GameObserver &observer, QObject *const parent)
    : QObject(parent)
    , m_library(library)
    , m_observer(observer)
{
    m_music = new MusicManager(m_library, this);
    m_sfx = new SfxManager(m_library, this);

    m_observer.sig2_gainedLevel.connect(m_lifetime, [this]() { m_sfx->playSound("level-up"); });

    setConfig().audio.registerChangeCallback(m_lifetime, [this]() {
        if constexpr (CURRENT_PLATFORM == PlatformEnum::Wasm) {
            if (getConfig().audio.isUnlocked()) {
                static std::once_flag flag;
                std::call_once(flag, [this]() {
                    // Browser sandbox needs a user interaction to unlock audio
                    playSound("level-up");
                    m_music->updateVolumes();
                });
            }
        }
        updateVolumes();
    });
}

AudioManager::~AudioManager() = default;

void AudioManager::onAreaChanged(const RoomArea &area)
{
    if (area.isEmpty()) {
        m_music->stopMusic();
        return;
    }

    static const QRegularExpression regex("^the\\s+");
    QString name = area.toQString().toLower().remove(regex).replace(' ', '-');
    mmqt::toAsciiInPlace(name);

    QString musicFile = m_library.findAudio("areas", name);
    m_music->playMusic(musicFile);
}

void AudioManager::playSound(const QString &soundName)
{
    m_sfx->playSound(soundName);
}

void AudioManager::updateVolumes()
{
    m_music->updateVolumes();
    m_sfx->updateVolume();
}
