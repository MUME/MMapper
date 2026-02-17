#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../global/macros.h"

#include <QObject>
#include <QSet>
#include <QString>

class MediaLibrary;
class QAudioOutput;

class NODISCARD_QOBJECT SfxManager final : public QObject
{
    Q_OBJECT

private:
#ifndef MMAPPER_NO_AUDIO
    QAudioOutput *m_output;
#endif
    const MediaLibrary &m_library;

public:
    explicit SfxManager(const MediaLibrary &library, QObject *parent = nullptr);

    void playSound(const QString &soundName);
    void updateVolume();
};
