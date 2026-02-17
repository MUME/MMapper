// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "audiopage.h"

#include "../configuration/configuration.h"
#include "ui_audiopage.h"

AudioPage::AudioPage(QWidget *const parent)
    : QWidget(parent)
    , ui(new Ui::AudioPage)
{
    ui->setupUi(this);

    slot_loadConfig();

    connect(ui->musicVolumeSlider,
            &QSlider::valueChanged,
            this,
            &AudioPage::slot_musicVolumeChanged);
    connect(ui->soundsVolumeSlider,
            &QSlider::valueChanged,
            this,
            &AudioPage::slot_soundsVolumeChanged);

    if constexpr (NO_AUDIO) {
        ui->musicVolumeSlider->setEnabled(false);
        ui->soundsVolumeSlider->setEnabled(false);
    }
}

AudioPage::~AudioPage()
{
    delete ui;
}

void AudioPage::slot_loadConfig()
{
    const auto &settings = getConfig().audio;
    ui->musicVolumeSlider->setValue(settings.getMusicVolume());
    ui->soundsVolumeSlider->setValue(settings.getSoundVolume());
}

void AudioPage::slot_musicVolumeChanged(int value)
{
    auto &settings = setConfig().audio;
    settings.setMusicVolume(value);
    settings.setUnlocked();
}

void AudioPage::slot_soundsVolumeChanged(int value)
{
    auto &settings = setConfig().audio;
    settings.setSoundVolume(value);
    settings.setUnlocked();
}
