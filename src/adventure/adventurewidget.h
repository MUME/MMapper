#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2023 The MMapper Authors
// Author: Mike Repass <mike.repass@gmail.com> (Taryn)

#include "../global/macros.h"
#include "AdventureLogModel.h"
#include "adventuretracker.h"

#include <QString>
#include <QWidget>
#include <QtCore>
#include <QtWidgets>

// Read-only text view of an AdventureLogModel: one document block per model
// row, kept in sync through the model's rowsInserted()/rowsRemoved()
// signals. The model owns the line cap and the AdventureTracker wiring.
class NODISCARD_QOBJECT AdventureWidget final : public QWidget
{
    Q_OBJECT

private:
    AdventureLogModel *m_model = nullptr;
    QTextEdit *m_textEdit = nullptr;
    QAction *m_clearContentAction = nullptr;

public:
    explicit AdventureWidget(AdventureTracker &at, QWidget *parent);

private:
    void appendRows(int first, int last);
    void removeRows(int first, int last);

private slots:
    void slot_rowsInserted(const QModelIndex &parent, int first, int last);
    void slot_rowsRemoved(const QModelIndex &parent, int first, int last);
    void slot_contextMenuRequested(const QPoint &pos);
};
