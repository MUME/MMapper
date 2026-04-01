#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../global/macros.h"

#include <QCache>
#include <QObject>
#include <QString>

#ifndef MMAPPER_NO_AUDIO
#include <QAudioDevice>
#endif

class MediaLibrary;
class QAudioOutput;
class QTemporaryFile;
class QTimer;
class QMediaPlayer;
class QUrl;

class NODISCARD_QOBJECT MusicManager final : public QObject
{
    Q_OBJECT

private:
    MediaLibrary &m_library;

#ifndef MMAPPER_NO_AUDIO
    struct MusicChannel
    {
        QMediaPlayer *player = nullptr;
        QAudioOutput *audioOutput = nullptr;
        QString file;
        qint64 pendingPosition = -1;
        float fadeVolume = 0.0f;
    };

    MusicChannel m_channels[2];
    int m_activeChannel = 0;
    QCache<QString, qint64> m_cachedPositions;
    QCache<QString, QTemporaryFile> m_wasmFiles;

    QTimer *m_fadeTimer = nullptr;
    float m_fadeStep = 0.05f;
    bool m_fadingToSilence = false;

    static constexpr int CROSSFADE_DURATION_MS = 2000;
    static constexpr int FADE_INTERVAL_MS = 100;
#endif

public:
    explicit MusicManager(MediaLibrary &library, QObject *parent = nullptr);
    ~MusicManager() override;

    void playMusic(const QString &musicFile);
    void stopMusic();

public:
    void updateVolumes();
#ifndef MMAPPER_NO_AUDIO
    void updateOutputDevice(const QAudioDevice &device);
#endif

public slots:
    void slot_onMediaChanged();

private:
#ifndef MMAPPER_NO_AUDIO
    NODISCARD int prepareChannel(const QString &musicFile);
    void activateChannel(int channelIndex, const QString &musicFile, const QUrl &source);
    void applyPendingPosition(int channelIndex);
    void updateChannelVolume(int channelIndex);
    void startFade(bool toSilence);
#endif
};
