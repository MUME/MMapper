#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 The MMapper Authors

#include "../global/macros.h"

#include <QAbstractNativeEventFilter>
#include <QByteArray>
#include <QObject>

class NODISCARD_QOBJECT WinDarkMode final : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    explicit WinDarkMode(QObject *parent = nullptr);
    ~WinDarkMode() override;

    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

    static bool isDarkMode();

signals:
    void sig_darkModeChanged(bool dark);

private:
    void applyCurrentPalette();
    void applyDarkPalette();
    void applyLightPalette();
};
