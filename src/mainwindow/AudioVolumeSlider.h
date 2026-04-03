#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../global/RuleOf5.h"
#include "../global/Signal2.h"
#include "../global/macros.h"

#include <QSlider>

class NODISCARD_QOBJECT AudioVolumeSlider final : public QSlider
{
    Q_OBJECT
    Q_PROPERTY(AudioType audioType READ audioType WRITE setAudioType)

public:
    enum class NODISCARD AudioType : uint8_t { Music, Sound };
    Q_ENUM(AudioType)

private:
    Signal2Lifetime m_lifetime;
    AudioType m_type = AudioType::Music;

public:
    explicit AudioVolumeSlider(QWidget *parent = nullptr);
    explicit AudioVolumeSlider(AudioType type, QWidget *parent = nullptr);
    ~AudioVolumeSlider() final;
    DELETE_CTORS_AND_ASSIGN_OPS(AudioVolumeSlider);

public:
    NODISCARD AudioType audioType() const { return m_type; }
    void setAudioType(AudioType type);

    void updateFromConfig();

protected:
    void wheelEvent(QWheelEvent *event) override;

private:
    void init();
    void updateToConfig(int value);
};
