// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "AudioVolumeSlider.h"

#include "../configuration/configuration.h"
#include "../global/SignalBlocker.h"

#include <QWheelEvent>

AudioVolumeSlider::AudioVolumeSlider(QWidget *const parent)
    : QSlider(Qt::Orientation::Horizontal, parent)
{
    init();
}

AudioVolumeSlider::AudioVolumeSlider(AudioType type, QWidget *const parent)
    : QSlider(Qt::Orientation::Horizontal, parent)
    , m_type(type)
{
    init();
}

AudioVolumeSlider::~AudioVolumeSlider() = default;

void AudioVolumeSlider::init()
{
    setRange(0, 100);
    connect(this, &QSlider::valueChanged, this, [this](int value) { updateToConfig(value); });

    setConfig().audio.registerChangeCallback(m_lifetime, [this]() { updateFromConfig(); });

    if constexpr (NO_AUDIO) {
        setEnabled(false);
    }
    setAudioType(m_type);
}

void AudioVolumeSlider::setAudioType(AudioType type)
{
    if (m_type == type && toolTip().length() > 0) {
        return;
    }

    m_type = type;
    updateFromConfig();

    switch (m_type) {
    case AudioType::Music:
        setToolTip(tr("Music Volume"));
        break;
    case AudioType::Sound:
        setToolTip(tr("Sound Volume"));
        break;
    }
}

void AudioVolumeSlider::updateFromConfig()
{
    int actualVolume = 0;
    switch (m_type) {
    case AudioType::Music:
        actualVolume = getConfig().audio.getMusicVolume();
        break;
    case AudioType::Sound:
        actualVolume = getConfig().audio.getSoundVolume();
        break;
    }

    if (value() != actualVolume) {
        const SignalBlocker block{*this};
        setValue(actualVolume);
    }
}

void AudioVolumeSlider::updateToConfig(int value)
{
    auto &audioSettings = setConfig().audio;
    switch (m_type) {
    case AudioType::Music:
        if (audioSettings.getMusicVolume() != value) {
            audioSettings.setMusicVolume(value);
            audioSettings.setUnlocked();
        }
        break;
    case AudioType::Sound:
        if (audioSettings.getSoundVolume() != value) {
            audioSettings.setSoundVolume(value);
            audioSettings.setUnlocked();
        }
        break;
    }
}

void AudioVolumeSlider::wheelEvent(QWheelEvent *event)
{
    event->ignore();
}
