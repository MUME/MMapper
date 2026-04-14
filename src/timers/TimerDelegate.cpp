// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2024 The MMapper Authors

#include "TimerDelegate.h"

#include "TimerModel.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QColor>
#include <QPainter>
#include <QTableView>

TimerDelegate::TimerDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{}

void TimerDelegate::paint(QPainter *painter,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const
{
    QVariant progressVar = index.data(TimerModel::ProgressRole);
    if (progressVar.isValid()) {
        double progress = progressVar.toDouble();

        painter->save();

        // Calculate progress bar width relative to the whole row
        const QTableView *view = qobject_cast<const QTableView *>(option.widget);
        if (view) {
            QRect rowRect;
            int colCount = view->model()->columnCount();
            for (int i = 0; i < colCount; ++i) {
                if (!view->isColumnHidden(i)) {
                    rowRect = rowRect.united(view->visualRect(view->model()->index(index.row(), i)));
                }
            }

            int totalWidth = rowRect.width();
            int filledWidth = static_cast<int>(static_cast<double>(totalWidth) * progress);
            QRect progressRect(rowRect.x(), rowRect.y(), filledWidth, rowRect.height());

            // Intersect row-level progress with current cell
            QRect cellProgressRect = progressRect.intersected(option.rect);

            if (cellProgressRect.isValid()) {
                QColor color;
                if (progress > 0.5) {
                    // Green to Yellow
                    double factor = (progress - 0.5) * 2.0;
                    color = QColor::fromRgbF(static_cast<float>(1.0 - factor), 1.0f, 0.0f, 0.3f);
                } else {
                    // Yellow to Red
                    double factor = progress * 2.0;
                    color = QColor::fromRgbF(1.0f, static_cast<float>(factor), 0.0f, 0.3f);
                }
                painter->fillRect(cellProgressRect, color);
            }
        }

        painter->restore();
    }

    QStyledItemDelegate::paint(painter, option, index);
}
