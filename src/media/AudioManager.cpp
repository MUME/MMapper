// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "AudioManager.h"

#include "../configuration/configuration.h"
#include "../global/Charset.h"
#include "../map/mmapper2room.h"
#include "MediaLibrary.h"
#include "MusicManager.h"
#include "SfxManager.h"

#ifndef MMAPPER_NO_AUDIO
#include <QAudioDevice>
#include <QMediaDevices>
#endif

#include <QRegularExpression>

AudioManager::AudioManager(MediaLibrary &library, GameObserver &observer, QObject *const parent)
    : QObject(parent)
    , m_library(library)
    , m_observer(observer)
{
    m_music = new MusicManager(m_library, this);
    m_sfx = new SfxManager(m_library, this);

    m_observer.sig2_gainedLevel.connect(m_lifetime, [this]() { playSound("level-up"); });

    setConfig().audio.registerChangeCallback(m_lifetime, [this]() {
        updateOutputDevices();
        m_sfx->updateVolume();
        if constexpr (CURRENT_PLATFORM == PlatformEnum::Wasm) {
            if (getConfig().audio.isUnlocked()) {
                static std::once_flag flag;
                std::call_once(flag, [this]() {
                    // Browser sandbox needs a user interaction to unlock audio
                    playSound("level-up");
                });
            }
        }
        m_music->updateVolumes();
    });

#ifndef MMAPPER_NO_AUDIO
    auto *const mediaDevices = new QMediaDevices(this);
    connect(mediaDevices,
            &QMediaDevices::audioOutputsChanged,
            this,
            &AudioManager::updateOutputDevices);
#endif

    updateOutputDevices();
    updateVolumes();
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

    m_music->playMusic(m_library.findAudio("areas", name));
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

void AudioManager::updateOutputDevices()
{
#ifndef MMAPPER_NO_AUDIO
    const QByteArray &deviceId = getConfig().audio.getOutputDeviceId();
    QAudioDevice targetDevice = QMediaDevices::defaultAudioOutput();

    if (!deviceId.isEmpty()) {
        const QList<QAudioDevice> devices = QMediaDevices::audioOutputs();
        for (const QAudioDevice &device : devices) {
            if (device.id() == deviceId) {
                targetDevice = device;
                break;
            }
        }
    }

    m_music->updateOutputDevice(targetDevice);
    m_sfx->updateOutputDevice(targetDevice);
#endif
}
