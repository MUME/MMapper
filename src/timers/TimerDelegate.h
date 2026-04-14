#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "../global/macros.h"

#include <QStyledItemDelegate>

class NODISCARD_QOBJECT TimerDelegate final : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit TimerDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
};
