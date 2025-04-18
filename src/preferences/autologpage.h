#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2019 The MMapper Authors
// Author: Mattias 'Mew_' Viklund <devmew@exedump.com> (Mirnir)

#include "../global/macros.h"

#include <QWidget>
#include <QtCore>

namespace Ui {
class AutoLogPage;
}

class NODISCARD_QOBJECT AutoLogPage final : public QWidget
{
    Q_OBJECT

private:
    Ui::AutoLogPage *const ui;

public:
    explicit AutoLogPage(QWidget *parent);
    ~AutoLogPage() final;

public slots:
    void slot_loadConfig();
    void slot_logStrategyChanged(int);
    void slot_selectLogLocationButtonClicked(int);
};
