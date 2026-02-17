#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 The MMapper Authors

#include "../global/macros.h"

#include <QObject>
#include <QWidget>

class QPushButton;
class QLabel;

class NODISCARD_QOBJECT AudioHintWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit AudioHintWidget(QWidget *parent = nullptr);
    ~AudioHintWidget() override;

private:
    QLabel *m_iconLabel;
    QLabel *m_textLabel;
    QPushButton *m_yesButton;
    QPushButton *m_noButton;
};
