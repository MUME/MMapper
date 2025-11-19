#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "displaywidget.h"

#include <QString>
#include <QTextEdit>

class NODISCARD_QOBJECT PreviewWidget final : public QTextEdit
{
    Q_OBJECT

private:
    AnsiTextHelper helper;

public:
    explicit PreviewWidget(QWidget *parent = nullptr);
    ~PreviewWidget() final = default;

    void init(const QFont &mainDisplayFont);

public:
    void displayText(const QString &fullText);
};
