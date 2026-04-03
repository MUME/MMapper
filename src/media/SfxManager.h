#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../global/RuleOf5.h"
#include "../global/macros.h"

#include <QObject>
#include <QSet>
#include <QString>

#ifndef MMAPPER_NO_AUDIO
#include <QAudioDevice>
#endif

class MediaLibrary;
class QAudioOutput;
class QUrl;

class NODISCARD_QOBJECT SfxManager final : public QObject
{
    Q_OBJECT

private:
#ifndef MMAPPER_NO_AUDIO
    QAudioOutput *m_output = nullptr;
#endif
    MediaLibrary &m_library;

public:
    explicit SfxManager(MediaLibrary &library, QObject *parent = nullptr);
    ~SfxManager() override;
    DELETE_CTORS_AND_ASSIGN_OPS(SfxManager);

    void shutdown();
    void playSound(const QString &soundName);

    void updateVolume();

#ifndef MMAPPER_NO_AUDIO
    void updateOutputDevice(const QAudioDevice &device);
#endif

private:
    void playFromData(const QByteArray &data, const QString &soundName);
    void startEffect(const QUrl &url, const QString &tmpToDelete = {});
};
