#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "../global/macros.h"

#include <QWidget>

namespace Ui {
class AudioPage;
}

class NODISCARD_QOBJECT AudioPage final : public QWidget
{
    Q_OBJECT

private:
    Ui::AudioPage *const ui;

public:
    explicit AudioPage(QWidget *parent);
    ~AudioPage() final;

public slots:
    void slot_loadConfig();
    void slot_musicVolumeChanged(int);
    void slot_soundsVolumeChanged(int);
};
