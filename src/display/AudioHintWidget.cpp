// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "AudioHintWidget.h"

#include "../configuration/configuration.h"

#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

AudioHintWidget::AudioHintWidget(QWidget *const parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 4, 10, 4);
    layout->setSpacing(8);

    m_iconLabel = new QLabel(this);
    m_iconLabel->setPixmap(QIcon(":/icons/audiocfg.png").pixmap(16, 16));
    layout->addWidget(m_iconLabel);

    m_textLabel = new QLabel(tr("Enable game audio?"), this);
    layout->addWidget(m_textLabel);

    m_yesButton = new QPushButton(tr("Yes"), this);

    m_noButton = new QPushButton(tr("No"), this);

    layout->addWidget(m_yesButton);
    layout->addWidget(m_noButton);
    layout->addStretch();

    connect(m_yesButton, &QPushButton::clicked, this, [this]() {
        setConfig().audio.setUnlocked();
        hide();
    });

    connect(m_noButton, &QPushButton::clicked, this, [this]() {
        // Set volumes to 0 if they choose "No"
        auto &settings = setConfig().audio;
        settings.setMusicVolume(0);
        settings.setSoundVolume(0);
        hide();
    });

    auto updateAudioHint = [this]() {
        auto &cfg = getConfig().audio;
        setVisible(!NO_AUDIO && !cfg.isUnlocked()
                   && (cfg.getMusicVolume() > 0 || cfg.getSoundVolume() > 0));
    };

    updateAudioHint();
    setConfig().audio.registerChangeCallback(m_lifetime, updateAudioHint);
}

AudioHintWidget::~AudioHintWidget() = default;
